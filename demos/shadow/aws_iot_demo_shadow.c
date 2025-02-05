
/* The config header is always included first. */
#include "iot_config.h"

/* Standard includes. */
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

/* Set up logging for this demo. */
#include "iot_demo_logging.h"

/* Platform layer includes. */
#include "platform/iot_clock.h"
#include "platform/iot_threads.h"

/* MQTT include. */
#include "iot_mqtt.h"

/* Shadow include. */
#include "aws_iot_shadow.h"

/* JSON utilities include. */
#include "iot_json_utils.h"

#include "FreeRTOSIPConfig.h"

/*Uart headers */
#include "FreeRTOS.h"
#include "task.h"
#include "driver/uart.h"
#include "aws_iot_shadow_blem.h"
/*
 * - Port: UART1
 * - Receive (Rx) buffer: on
 * - Transmit (Tx) buffer: off
 * - Flow control: off
 * - Event queue: off
 * - Pin assignment: see defines below
*/
#define ECHO_TEST_TXD (GPIO_NUM_16)
#define ECHO_TEST_RXD (GPIO_NUM_17)
#define ECHO_TEST_RTS (UART_PIN_NO_CHANGE)
#define ECHO_TEST_CTS (UART_PIN_NO_CHANGE)

#define BUF_SIZE (1024)

/**
 * Provide default values for undefined configuration settings.
 */

#define SHADOW_UPDATE_PERIOD_MS (2000)

#define SHADOW_GET_PERIOD_MS (2000)


/* Validate Shadow demo configuration settings. */

#if SHADOW_UPDATE_PERIOD_MS <= 0
#error "SHADOW_UPDATE_PERIOD_MS cannot be 0 or negative."
#endif

/**
 * @brief The keep-alive interval used for this demo.
 *
 * An MQTT ping request will be sent periodically at this interval.
 */
#define KEEP_ALIVE_SECONDS (600)

/**
 * @brief The timeout for Shadow and MQTT operations in this demo.
 */
#define TIMEOUT_MS (20000)


/*-----------------------------------------------------------*/
static void uart_init()
{
    /* Configure parameters of an UART driver,
     * communication pins and install the driver */
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE};
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, ECHO_TEST_TXD, ECHO_TEST_RXD, ECHO_TEST_RTS, ECHO_TEST_CTS);
    uart_driver_install(UART_NUM_1, BUF_SIZE * 2,BUF_SIZE * 2, 0, NULL, 0);

    
}

/*-----------------------------------------------------------*/

/* Declaration of demo function. */
int RunShadowDemo(bool awsIotMqttMode,
                  const char *pIdentifier,
                  void *pNetworkServerInfo,
                  void *pNetworkCredentialInfo,
                  const IotNetworkInterface_t *pNetworkInterface);


/*-----------------------------------------------------------*/

/**
 * @brief Initialize the the MQTT library and the Shadow library.
 *
 * @return `EXIT_SUCCESS` if all libraries were successfully initialized;
 * `EXIT_FAILURE` otherwise.
 */


static int _initializeDemo(void)
{
    int status = EXIT_SUCCESS;
    IotMqttError_t mqttInitStatus = IOT_MQTT_SUCCESS;
    AwsIotShadowError_t shadowInitStatus = AWS_IOT_SHADOW_SUCCESS;

    /* Flags to track cleanup on error. */
    bool mqttInitialized = false;

    /* Initialize the MQTT library. */
    mqttInitStatus = IotMqtt_Init();

    if (mqttInitStatus == IOT_MQTT_SUCCESS)
    {
        mqttInitialized = true;
    }
    else
    {
        status = EXIT_FAILURE;
    }

    /* Initialize the Shadow library. */
    if (status == EXIT_SUCCESS)
    {
        /* Use the default MQTT timeout. */
        shadowInitStatus = AwsIotShadow_Init(0);

        if (shadowInitStatus != AWS_IOT_SHADOW_SUCCESS)
        {
            status = EXIT_FAILURE;
        }
    }

    /* Clean up on error. */
    if (status == EXIT_FAILURE)
    {
        if (mqttInitialized == true)
        {
            IotMqtt_Cleanup();
        }
    }

    return status;
}

/*-----------------------------------------------------------*/

/**
 * @brief Clean up the the MQTT library and the Shadow library.
 */
static void _cleanupDemo(void)
{
    AwsIotShadow_Cleanup();
    IotMqtt_Cleanup();
}

/*-----------------------------------------------------------*/

/**
 * @brief Establish a new connection to the MQTT server for the Shadow demo.
 *
 * @param[in] pIdentifier NULL-terminated MQTT client identifier. The Shadow
 * demo will use the Thing Name as the client identifier.
 * @param[in] pNetworkServerInfo Passed to the MQTT connect function when
 * establishing the MQTT connection.
 * @param[in] pNetworkCredentialInfo Passed to the MQTT connect function when
 * establishing the MQTT connection.
 * @param[in] pNetworkInterface The network interface to use for this demo.
 * @param[out] pMqttConnection Set to the handle to the new MQTT connection.
 *
 * @return `EXIT_SUCCESS` if the connection is successfully established; `EXIT_FAILURE`
 * otherwise.
 */
static int _establishMqttConnection(const char *pIdentifier,
                                    void *pNetworkServerInfo,
                                    void *pNetworkCredentialInfo,
                                    const IotNetworkInterface_t *pNetworkInterface,
                                    IotMqttConnection_t *pMqttConnection)
{
    int status = EXIT_SUCCESS;
    IotMqttError_t connectStatus = IOT_MQTT_STATUS_PENDING;
    IotMqttNetworkInfo_t networkInfo = IOT_MQTT_NETWORK_INFO_INITIALIZER;
    IotMqttConnectInfo_t connectInfo = IOT_MQTT_CONNECT_INFO_INITIALIZER;

    if (pIdentifier == NULL)
    {
        IotLogError("Shadow Thing Name must be provided.");

        status = EXIT_FAILURE;
    }

    if (status == EXIT_SUCCESS)
    {
        /* Set the members of the network info not set by the initializer. This
         * struct provided information on the transport layer to the MQTT connection. */
        networkInfo.createNetworkConnection = true;
        networkInfo.u.setup.pNetworkServerInfo = pNetworkServerInfo;
        networkInfo.u.setup.pNetworkCredentialInfo = pNetworkCredentialInfo;
        networkInfo.pNetworkInterface = pNetworkInterface;

#if (IOT_MQTT_ENABLE_SERIALIZER_OVERRIDES == 1) && defined(IOT_DEMO_MQTT_SERIALIZER)
        networkInfo.pMqttSerializer = IOT_DEMO_MQTT_SERIALIZER;
#endif

        /* Set the members of the connection info not set by the initializer. */
        connectInfo.awsIotMqttMode = true;
        connectInfo.cleanSession = true;
        connectInfo.keepAliveSeconds = KEEP_ALIVE_SECONDS;

        /* AWS IoT recommends the use of the Thing Name as the MQTT client ID. */
        connectInfo.pClientIdentifier = pIdentifier;
        connectInfo.clientIdentifierLength = (uint16_t)strlen(pIdentifier);

        IotLogInfo("Shadow Thing Name is %.*s (length %hu).",
                   connectInfo.clientIdentifierLength,
                   connectInfo.pClientIdentifier,
                   connectInfo.clientIdentifierLength);

        size_t connectNum = 0;
        /* Establish the MQTT connection. */
        while(connectStatus != IOT_MQTT_SUCCESS)
        {
            connectStatus = IotMqtt_Connect(&networkInfo,
                                        &connectInfo,
                                        10000,
                                        pMqttConnection);
            if (connectStatus != IOT_MQTT_SUCCESS)
            {
                connectNum++;
                IotLogError("MQTT CONNECT returned error %s. connect number %d",
                            IotMqtt_strerror(connectStatus),connectNum);
            }
        }   
    }

    return status;
}

/*-----------------------------------------------------------*/

/**
 * @brief Parses the "state" key from the "previous" or "current" sections of a
 * Shadow updated document.
 *
 * @param[in] pUpdatedDocument The Shadow updated document to parse.
 * @param[in] updatedDocumentLength The length of `pUpdatedDocument`.
 * @param[in] pSectionKey Either "previous" or "current". Must be NULL-terminated.
 * @param[out] pState Set to the first character in "state".
 * @param[out] pStateLength Length of the "state" section.
 *
 * @return `true` if the "state" was found; `false` otherwise.
 */
// static bool _getUpdatedState( const char * pUpdatedDocument,
//                               size_t updatedDocumentLength,
//                               const char * pSectionKey,
//                               const char ** pState,
//                               size_t * pStateLength )
// {
//     bool sectionFound = false, stateFound = false;
//     const size_t sectionKeyLength = strlen( pSectionKey );
//     const char * pSection = NULL;
//     size_t sectionLength = 0;

//     /* Find the given section in the updated document. */
//     sectionFound = IotJsonUtils_FindJsonValue( pUpdatedDocument,
//                                                updatedDocumentLength,
//                                                pSectionKey,
//                                                sectionKeyLength,
//                                                &pSection,
//                                                &sectionLength );

//     if( sectionFound == true )
//     {
//         /* Find the "state" key within the "previous" or "current" section. */
//         stateFound = IotJsonUtils_FindJsonValue( pSection,
//                                                  sectionLength,
//                                                  "state",
//                                                  5,
//                                                  pState,
//                                                  pStateLength );
//     }
//     else
//     {
//         IotLogWarn( "Failed to find section %s in Shadow updated document.",
//                     pSectionKey );
//     }

//     return stateFound;
// }


/*-----------------------------------------------------------*/

// /**
//  * @brief Shadow updated callback, invoked when the Shadow document changes.
//  *
//  * This function reports when a Shadow has been updated.
//  *
//  * @param[in] pCallbackContext Not used.
//  * @param[in] pCallbackParam The received Shadow updated document.
//  */
// static void _shadowUpdatedCallback( void * pCallbackContext,
//                                     AwsIotShadowCallbackParam_t * pCallbackParam )
// {
//     bool previousFound = false, currentFound = false;
//     const char * pPrevious = NULL, * pCurrent = NULL;
//     size_t previousLength = 0, currentLength = 0;

//     /* Silence warnings about unused parameters. */
//     ( void ) pCallbackContext;

//     /* Find the previous Shadow document. */
//     previousFound = _getUpdatedState( pCallbackParam->u.callback.pDocument,
//                                       pCallbackParam->u.callback.documentLength,
//                                       "previous",
//                                       &pPrevious,
//                                       &previousLength );

//     /* Find the current Shadow document. */
//     currentFound = _getUpdatedState( pCallbackParam->u.callback.pDocument,
//                                      pCallbackParam->u.callback.documentLength,
//                                      "current",
//                                      &pCurrent,
//                                      &currentLength );

//     /* Log the previous and current states. */
//     if( ( previousFound == true ) && ( currentFound == true ) )
//     {
//         IotLogInfo( "Shadow was updated!\r\n"
//                     "Previous: {\"state\":%.*s}\r\n"
//                     "Current:  {\"state\":%.*s}",
//                     30,
//                     pPrevious,
//                     30,
//                     pCurrent );
//     }
//     else
//     {
//         if( previousFound == false )
//         {
//             IotLogWarn( "Previous state not found in Shadow updated document." );
//         }

//         if( currentFound == false )
//         {
//             IotLogWarn( "Current state not found in Shadow updated document." );
//         }
//     }
// }
/**
 * @brief Shadow delta callback, invoked when the desired and updates Shadow
 * states differ.
 *
 * This function simulates a device updating its state in response to a Shadow.
 *
 * @param[in] pCallbackContext Not used.
 * @param[in] pCallbackParam The received Shadow delta document.
 */
static void _shadowDeltaCallback( void * pCallbackContext,
                                  AwsIotShadowCallbackParam_t * pCallbackParam )
{
    bool deltaFound = false;
    bool onOffFound = false;
    IotSemaphore_t * pDeltaSemaphore = pCallbackContext;

    const char * pDelta = NULL;
    size_t deltaLength = 0;
    IotLogInfo("Received Document is:\r\n %.*s\r\n",pCallbackParam->u.callback.documentLength,
                                               pCallbackParam->u.callback.pDocument);
    /* Check if there is a different "ON_OFF" state in the Shadow. */
    deltaFound = IotJsonUtils_FindJsonValue( pCallbackParam->u.callback.pDocument,
                                            pCallbackParam->u.callback.documentLength,
                                            "state",
                                            5,
                                            &pDelta,
                                            &deltaLength );

    IotLogInfo("\r\n");
    IotLogInfo("pDelta is %.*s\r\n",deltaLength,pDelta);
    if(deltaFound == true)
    {   
        onOffFound = IotJsonUtils_FindJsonValue( pDelta,
                                                deltaLength,
                                                "ON_OFF",
                                                6,
                                                NULL,
                                                NULL);

        if(onOffFound == true)
        {
        //write extracted command to uart
        _write_command_into_uart(pDelta,deltaLength);
        }
    /* Post to the delta semaphore to unblock the thread sending Shadow updates. */
    IotSemaphore_Post( pDeltaSemaphore );
    }
}

static int _setShadowCallbacks( IotSemaphore_t * pDeltaSemaphore,
                                IotMqttConnection_t mqttConnection,
                                const char * pThingName,
                                size_t thingNameLength )
{
    int status = EXIT_SUCCESS;
    AwsIotShadowError_t callbackStatus = AWS_IOT_SHADOW_STATUS_PENDING;
    AwsIotShadowCallbackInfo_t deltaCallback = AWS_IOT_SHADOW_CALLBACK_INFO_INITIALIZER;
                            //    updatedCallback = AWS_IOT_SHADOW_CALLBACK_INFO_INITIALIZER;

    /* Set the functions for callbacks. */
    deltaCallback.pCallbackContext = pDeltaSemaphore;
    deltaCallback.function = _shadowDeltaCallback;
    // updatedCallback.function = _shadowUpdatedCallback;

    /* Set the delta callback, which notifies of different desired and reported
     * Shadow states. */
    callbackStatus = AwsIotShadow_SetDeltaCallback( mqttConnection,
                                                    pThingName,
                                                    thingNameLength,
                                                    0,
                                                    &deltaCallback );

    // if( callbackStatus == AWS_IOT_SHADOW_SUCCESS )
    // {
    //     /* Set the updated callback, which notifies when a Shadow document is
    //      * changed. */
    //     callbackStatus = AwsIotShadow_SetUpdatedCallback( mqttConnection,
    //                                                       pThingName,
    //                                                       thingNameLength,
    //                                                       0,
    //                                                       &updatedCallback );
    // }

    if( callbackStatus != AWS_IOT_SHADOW_SUCCESS )
    {
        IotLogError( "Failed to set demo shadow callback, error %s.",
                     AwsIotShadow_strerror( callbackStatus ) );

        status = EXIT_FAILURE;
    }

    return status;
}

/*------------------------------------------------------------------------*/

/*get specific value in the shadow document
the maximum nested layers for the shadow document is 4
shadow document format should be followed as below:
    "{"                                                                \
    "\"state\":{"                                                    \
        "\"desired\": {"                                             \
            "\"Lights\" :{"                                          \
                "\"thing name\" : \"sample-light\","                  \
                "\"device info\":\"000\","                          \
                "\"ON_OFF\":\"%s\","                                \
                "\"brightness\":\"%s\",                             \
                "\"value\" :  {\"value\": \"%s\" },"                \
                "\"property1\" : {\"default property1\": 0 },"      \
                "\"colorTemperatureInKelvin\" : \"%s\""             \
                "},"                                                \
            "\"Switch\":{"                                          \
                "\"Switch value\": \"%s\""                          \
            "},"                                                    \
            "\"Lock\":{"                                            \
                "\"Lock value\": \"%s\""                            \
            "}"                                                     \
        "}"                                                         \
    "}"                                                             \
    "\"clientToken\":\"%06lu\""                                     \
"}"                                                                 \  
    */
/**
 * The shadow document must be 4 nested layer like showed above, if you want to change
 * the structure of the shadow document, you need not only change this function, but also
 * change alexa skill and iot console document 
 * param receivedDocument Received shadow document from AwsIot_Get
 * param receivedDocumentLength Received shadow document length
 * param sectionId The section name, ie.state, delta etc
 * param desiredOrReportedId Desired or reported wanted
 * param attributeId Attribute wanted
 * param attributeGot [out] A pointer points to the attribute value got
 * param attributeLen [out] A pointer points to the length value
 */
static bool _getSpecificValue(const char* receivedDocument,
                              size_t receivedDocumentLength,
                              const char* sectionId,
                              const char* desiredOrReportedId,
                              const char* deviceNameId,
                              const char* attributeId,
                              const char** attributeGot,
                              size_t *attributeLen)
{
    const char *stateSection = NULL;
    size_t stateSectionLen =0;
    bool sectionFound = false;
    size_t sectionKeyLen  = strlen(sectionId);
    

    /* Find the "state" key in the shadow document. */
    sectionFound = IotJsonUtils_FindJsonValue(receivedDocument,
                                            receivedDocumentLength,
                                            sectionId,
                                            sectionKeyLen,
                                            &stateSection,
                                            &stateSectionLen);
    // IotLogInfo("state section is %s",stateSection);
    if(sectionFound)
    {
        // IotLogInfo("stateSectionFound");
        const char *desiredSection = NULL;
        size_t desiredSectionLength = 0;
        bool desiredFound = false;

        size_t desiredKeyLength = strlen(desiredOrReportedId);
    
        /* Find the delta key within the "desired" section. */
        desiredFound = IotJsonUtils_FindJsonValue(  stateSection,
                                                    stateSectionLen,
                                                    desiredOrReportedId,
                                                    desiredKeyLength,
                                                    &desiredSection,
                                                    &desiredSectionLength);
        
        if(desiredFound)
        {
            // IotLogInfo("desiredSectionFound");
            const char *deviceState = NULL;
            size_t deviceStateLen =0;
            bool deviceFound = false;

            const size_t deviceKeyLength = strlen(deviceNameId);

            
                /* Find the delta key within the "device" section. */
            deviceFound = IotJsonUtils_FindJsonValue(   desiredSection,
                                                        desiredSectionLength,
                                                        deviceNameId,
                                                        deviceKeyLength,
                                                        &deviceState,
                                                        &deviceStateLen);
            
            if(deviceFound)
            {
                // IotLogInfo("deviceFound");
                bool attributeFound = false;
                const size_t attributeKeyLength = strlen(attributeId);

                /* Find the delta key within the "attribute" section. */
                attributeFound = IotJsonUtils_FindJsonValue(    deviceState,
                                                                deviceStateLen,
                                                                attributeId,
                                                                attributeKeyLength,
                                                                attributeGot,
                                                                attributeLen);
                if(attributeFound)
                {
                    IotLogInfo("attributeFound");
                }
                else
                {IotLogInfo("attribute didn't find");}
                return attributeFound; 
            }
            else
            {IotLogWarn("device didn't find");}
            return deviceFound;
        }
        else
        {IotLogWarn("desired didn't find");}
        return desiredFound;
    }
    else
    {IotLogInfo("state section didn't find!");}
    return sectionFound;
}

/**
 * write value to the uart port 
 * @param command  the value to be written into uart port
 *  */
static void _write_command_into_uart(const char* command, size_t commandLength)
{
    IotLogInfo(" command is :%.*s ",commandLength,command);

    char value[commandLength+1];
    //clear buffer but left the last one 
    memset(value,'\0',commandLength*sizeof(char));

    strncpy(value,command,commandLength);
    int result = 0;

    //set the last bit of the buffer as '\n' to trigger the bg13
    value[commandLength]='\n';

    //write the data to uart
    result = uart_write_bytes(UART_NUM_1, (const char*)value, sizeof(value));

    if(result == -1)
    {
        IotLogInfo("Write command %s failed",value);
    }
    else
    {
        IotLogInfo("Write command to uart port successful! the data is :%.*s\n",commandLength,value);
    }
        //clear buffer but left the last one 
    memset(value,'\0',commandLength*sizeof(value[0]));
    
    return;
}


/*Get thing shadow from iot */
static int _thingShadowOperation( IotSemaphore_t * pDeltaSemaphore,
                                IotMqttConnection_t mqttConnection,
                                const char * pThingName,
                                size_t thingNameLength
                            )
{
    int status = EXIT_SUCCESS;
    /*using a while loop to continuously running the program */
    while(1)
    {
        //check if local has command to send to update the shadow document
        uint32_t length = uart_get_buffered_data_len(UART_NUM_1, (size_t*)&length);
        ESP_ERROR_CHECK(uart_get_buffered_data_len(UART_NUM_1, (size_t*)&length));          
        
        if( IotSemaphore_TimedWait( pDeltaSemaphore, 1000) == false )
        {
            if(( IotClock_GetTimeMs()/10000) == 0)
            {
                IotLogInfo( "No changes at %06d s",( long unsigned ) ( IotClock_GetTimeMs()/1000) );
            }
        }

        if(length!=0)
        {  
            status = reportLocalChange( length,
                                        mqttConnection,
                                        pThingName,
                                        thingNameLength,
                                        status);
            if(status != EXIT_SUCCESS)
            {
                IotLogInfo("Report local change failed");
                printf("Restarting now...\n");
                esp_restart();
                break;
            }  
        }
        
    }
    IotLogInfo("left getThingshadow function, the status now is %d",status);
    return status;
}


/**
 * report the local changes to cloud, if the button on the switch is pressed,
 * then update the shadow document on the cloud
 */
static int reportLocalChange(   uint32_t length,
                                IotMqttConnection_t mqttConnection,
                                const char * pThingName,
                                size_t thingNameLength,
                                int status)
{
    /** check if there is message in the rx buffer. if there is, analysis the data and update thing shadow */

 
    uint8_t *data = (uint8_t *) malloc(length);
    length = uart_read_bytes(UART_NUM_1, data, length, 100/portTICK_PERIOD_MS);
    IotLogInfo("Read from rx buffer:%.*s, length is %d",length,data,length);

    //analysis the operation type represented by the data received from uart
    //extract information from the data packet sent from bg13

    /*get the update operation type */
    UpdateOperation_t operation = analysisOperation(data);
    /*get the deviceType */
    Device_t deviceType = analysisDeviceType(data);
    /*get the attribute */
    Attribute_t attributeType = analysisAttribute(data);

    /**get device name from data*/
    //TODO increase the packet length 
    // char *deviceName = getDeviceNameFromPacket(data);
    // IotLogInfo("device name is %s: ",deviceName);

     /**get device attribute name from data*/
    // char *attributeName = getAttributeNameFromPacket(data);
    // IotLogInfo("attribute name is %s: ",deviceName);

    /*get the attribute value from data */
    char *attributeValue = _getAttributeValue(attributeType, data);
    char* pUpdateDocument = NULL;


    //generate the shadow document to send plus 20 to avoid data overflow
    pUpdateDocument = generateControlShadowDocument(operation, 
                                                    deviceType,
                                                    attributeType,
                                                    attributeValue
                                                    );

    
    AwsIotShadowError_t updateResult = AWS_IOT_SHADOW_STATUS_PENDING;
    updateResult = wrapUpdateThingShadow(   pUpdateDocument,
                                            mqttConnection,
                                            pThingName,
                                            thingNameLength );
    
    if( updateResult != AWS_IOT_SHADOW_SUCCESS )
    {
        IotLogError( "thing shadow update error %s.", AwsIotShadow_strerror( updateResult ) );
        status = EXIT_FAILURE;
    }
    else
    {
        IotLogInfo( "Successfully sent Shadow update ");
        esp_err_t success = uart_flush_input(UART_NUM_1);
        if(success != ESP_OK)
        {
            IotLogWarn("UART clear failed");
        }

    }
    free(data);
    return status;
}

/**
 * the length of each block should be showed as packet defined   
 */
static char* _getAttributeValue(Attribute_t attributeType, uint8_t *data)
{
    static char attributeValue[attributeValueLength] ={ '\0' };
    memset(attributeValue,'\0',attributeValueLength*sizeof(char));
    int length = 0;
    
    if(attributeType==ON_OFF)
    {
        //decide is "ON" or "OFF"
        if(data[31] =='O' && data[32]=='N'  )
            length = 2;
        else
            length = 3;
    }
    if(attributeType == LOCK_UNLOCK)
    {
        if(data[31] == 'L' && data[32]=='O')
        {
            length = 4;
        } 
        else
        {
            length = 6;
        }
    }

    //get third block of the data packet into attributeValue
    strncpy(  attributeValue,(const char*)(data                        \
                                +operationTypeLength                   \
                                +deviceNameLength                      \
                                +attributeNameLength),                 \
                                length);
    IotLogInfo("attribute value is %s", attributeValue);

    return attributeValue;
}

// static char* getDeviceNameFromPacket(uint8_t* data)
// {
//     static char deviceName[deviceNameLength]={'\0'};
//     memset(deviceName,'\0',deviceNameLength*sizeof(char));
//     int i =1;
//     while(data[i] != 'x')
//     {
//         i++;
//     }
//     strncpy(deviceName,(const char*)(data+1), i);

//     IotLogInfo("Get DeviceName is :%s",deviceName);

//     return deviceName;
// }
// static char* getAttributeNameFromPacket(uint8_t* data)
// {
//     static char attributeName[attributeNameLength]={'\0'};
//     memset(attributeName,'\0',attributeNameLength*sizeof(char));
//     int i =deviceNameLength+attributeNameLength;
//     while(data[i] != 'x')
//     {
//         i++;
//     }
//     strncpy(attributeName,(const char*)(data+deviceNameLength), i);
//     IotLogInfo("Get attributeName is :%s",attributeName);

//     return attributeName;
// }
/**
 * analysis the received data from UART and give corresponding updating operation type
 * type includes adding devices to shadow document
 * change device 
 */
static UpdateOperation_t analysisOperation(uint8_t* data)
{
    UpdateOperation_t type = UNKNOWN_OP;
    if(data[0]=='1')
    {
        type = CLOULD_CHANGE_ENDPOINT_STATE; 
        IotLogInfo("the type is CLOULD_CHANGE_ENDPOINT_STATE");  
    }
    if (data[0]=='2')
    {
        type = LOCALLY_CHANGE_ENDPOINT_STATE;
        IotLogInfo("the type is LOCALLY_CHANGE_ENDPOINT_STATE");  
    }

    return type;
}

/**
 * |----1--------|------10----|--------20------|--------10-------|
 * |  operation  |device type | attribute name | attribute value| 
 * the length of each block should be showed as above   
 */
/**Analysis device type that the data sent from esp32 has */
static Device_t analysisDeviceType(uint8_t* data)
{
    char deviceType[deviceNameLength]={'\0'};
    Device_t type = UNKNOWN_TYPE;
    
    //copy the first 10 characters
    int i =1;
    while(data[i]!='x')
    {
        deviceType[i-1]=data[i];
        i++;
    }

    if(strcmp((const char*)deviceType,"Lights")==0)
        {
            type = LIGHT;
            IotLogInfo("the device type is LIGHT"); 
        }
    else if (strcmp((const char*)deviceType,"Switch")==0)
        {
            type = SWITCH;
            IotLogInfo("the device type is SWITCH");
        }
    else if (strcmp((const char*)deviceType,"Lock")==0)
        {
            type = LOCK;
            IotLogInfo("the device type is LOCK");
           
        }
    else{
            type = UNKNOWN_TYPE;
        }

    return type;
}
/**
 * Analysis what attribute the endpoint want to update 
 * return the attribute type received from the packet
 * the length of each block should be showed as above   
 * 
 * The attribute codes available are: ON_OFF,LOCK_UNLOCK,POWER_LEVEL
 */
static Attribute_t analysisAttribute(uint8_t* data)
{
    char attribute[15] ={'\0'};
    Attribute_t att = UNKNOWN_ATT;
    //
    int i = operationTypeLength + deviceNameLength;
    int j = 0;
    while(data[i]!='x')
    {
        attribute[j]=data[i];
        i++;
        j++;
    }
    IotLogInfo("the device attribute is %s",attribute); 
    if (strcmp((const char*)attribute,"ON_OFF")==0)
    {
        att = ON_OFF;
        IotLogInfo("the attribute type is ON_OFF");
    }
    else if (strcmp((const char*)attribute,"LOCK_UNLOCK")==0)
    {
        /* code */
        att = LOCK_UNLOCK;
        IotLogInfo("the attribute type is LOCK_UNLOCK");
    }
    else if (strcmp((const char*)attribute,"POWER_LEVEL") == 0)
    {
        /* code */
        att = POWER_LEVEL;
        IotLogInfo("the attribute type is POWER_LEVEL");
    }

    return att;
}

/**
 * generate shadow document if the data analysis result is a change state directive
 * everytime a specific attribute value changes but the rest are not, then the rest attributes
 * should be remain in their default value
 */
static char* generateControlShadowDocument( UpdateOperation_t operation,
                                            Device_t deviceType, 
                                            Attribute_t attributeType,
                                            char *attributeValue
                                        )
{
    int length = 0;

    static char pUpdateDocument[ SHADOW_REPORTED_JSON_SIZE + SHADOW_DESIRED_JSON_SIZE +20 ] = { 0 };
    if(operation == LOCALLY_CHANGE_ENDPOINT_STATE){
        if(deviceType == LIGHT && attributeType == ON_OFF)
        {
            length = sprintf(   pUpdateDocument,
                                SHADOW_LIGHT_JSON,
                                attributeValue,
                                (int)D_Temperature,
                                attributeValue,
                                (int)D_Temperature,
                                ( long unsigned ) ( IotClock_GetTimeMs() % 1000000 ) ); //the clienToken

            IotLogInfo("document generated is %.*s: ",length,pUpdateDocument);
        }
        if(deviceType == LIGHT && attributeType == TEMPERATURE)
        {
            length = sprintf(   pUpdateDocument,
                                SHADOW_LIGHT_JSON,
                                "ON",
                                atoi(attributeValue),
                                "ON",
                                atoi(attributeValue),
                                ( long unsigned ) ( IotClock_GetTimeMs() % 1000000 ) ); //the clienToken

            IotLogInfo("document generated is %.*s: ",length,pUpdateDocument);
        }            
    }
    else if(operation == CLOULD_CHANGE_ENDPOINT_STATE){
        if(deviceType == LIGHT && attributeType == ON_OFF)
        {
            length = sprintf(   pUpdateDocument,
                                SHADOW_REPORTED_LIGHT_JSON,
                                attributeValue,
                                (int)D_Temperature,
                                ( long unsigned ) ( IotClock_GetTimeMs() % 1000000 ) ); //the clienToken

            IotLogInfo("document generated is %.*s: ",length,pUpdateDocument);
        }
        if(deviceType == LIGHT && attributeType == TEMPERATURE)
        {
            length = sprintf(   pUpdateDocument,
                                SHADOW_REPORTED_LIGHT_JSON,
                                "ON",
                                atoi(attributeValue),
                                ( long unsigned ) ( IotClock_GetTimeMs() % 1000000 ) ); //the clienToken

            IotLogInfo("document generated is %.*s: ",length,pUpdateDocument);
        }
    }

    return pUpdateDocument;
}
/**
 * generate shadow document if the data analysis result is a add device directive
 */
// static char* generateAddDeviceShadowDocument(   Device_t deviceType, 
//                                                 char* deviceName,
//                                                 Attribute_t attributeType,
//                                                 char* attributeName,
//                                                 char *attributeValue
//                                             )
// {
//     int length = 0;
//     static char pUpdateDocument[ DESIRED_ADD_DEVICE_STRING_ATTRIBUTE_SIZE + 20 ] = { 0 };
//     if((deviceType == LIGHT || deviceType == SWITCH) && attributeType == ON_OFF)
//     {
//         length = sprintf(   pUpdateDocument,
//                             DESIRED_ADD_DEVICE_STRING_ATTRIBUTE_JSON,
//                             deviceName,
//                             attributeName,
//                             attributeValue,
//                             ( long unsigned ) ( IotClock_GetTimeMs() % 1000000 ) ); //the clienToken

//         IotLogInfo("document generated is %.*s: ",length,pUpdateDocument);
//     }
//     if((deviceType == LOCK) && (attributeType == LOCK_UNLOCK))
//     {
//         length = sprintf(   pUpdateDocument,
//                             DESIRED_ADD_DEVICE_STRING_ATTRIBUTE_JSON,
//                             deviceName,
//                             attributeName,
//                             attributeValue,
//                             ( long unsigned ) ( IotClock_GetTimeMs() % 1000000 ) ); //the clienToken

//         IotLogInfo("document generated is %.*s: ",length,pUpdateDocument);
//     }

//     return pUpdateDocument;
// }


//update desired part thing shadow, only called when device has data coming 
static AwsIotShadowError_t wrapUpdateThingShadow(   char *pUpdateDocument,
                                                IotMqttConnection_t mqttConnection,
                                                const char * const pThingName,
                                                size_t thingNameLength )
{
    AwsIotShadowError_t updateStatus = AWS_IOT_SHADOW_STATUS_PENDING;
    AwsIotShadowDocumentInfo_t updateDocument = AWS_IOT_SHADOW_DOCUMENT_INFO_INITIALIZER;

    /* Set the common members of the Shadow update document info. */
    updateDocument.pThingName = pThingName;
    updateDocument.thingNameLength = thingNameLength;
    updateDocument.u.update.pUpdateDocument = pUpdateDocument;
    updateDocument.u.update.updateDocumentLength = strlen( updateDocument.u.update.pUpdateDocument );

    /* Send the Shadow update. Because the Shadow is constantly updated in
    * this demo, the "Keep Subscriptions" flag is passed to this function.
    * Note that this flag only needs to be passed on the first call, but
    * passing it for subsequent calls is fine.
    */
    updateStatus = AwsIotShadow_TimedUpdate(    mqttConnection,
                                                &updateDocument,
                                                AWS_IOT_SHADOW_FLAG_KEEP_SUBSCRIPTIONS,
                                                TIMEOUT_MS );
    
    return updateStatus;
}


/*-----------------------------------------------------------*/

/**
 * @brief The function that runs the Shadow demo, called by the demo runner.
 *
 * @param[in] awsIotMqttMode Ignored for the Shadow demo.
 * @param[in] pIdentifier NULL-terminated Shadow Thing Name.
 * @param[in] pNetworkServerInfo Passed to the MQTT connect function when
 * establishing the MQTT connection for Shadows.
 * @param[in] pNetworkCredentialInfo Passed to the MQTT connect function when
 * establishing the MQTT connection for Shadows.
 * @param[in] pNetworkInterface The network interface to use for this demo.
 *
 * @return `EXIT_SUCCESS` if the demo completes successfully; `EXIT_FAILURE` otherwise.
 */
int RunShadowDemo(bool awsIotMqttMode,
                  const char *pIdentifier,
                  void *pNetworkServerInfo,
                  void *pNetworkCredentialInfo,
                  const IotNetworkInterface_t *pNetworkInterface)
{


    
    /** initialize the uart  */
    uart_init();
    /* Return value of this function and the exit status of this program. */
    int status = 0;

    /* Handle of the MQTT connection used in this demo. */
    IotMqttConnection_t mqttConnection = IOT_MQTT_CONNECTION_INITIALIZER;

    /* Length of Shadow Thing Name. */
    size_t thingNameLength = 0;

    /* Allows the Shadow update function to wait for the delta callback to complete
     * a state change before continuing. */
    IotSemaphore_t deltaSemaphore;

    /* Flags for tracking which cleanup functions must be called. */
    bool librariesInitialized = false, connectionEstablished = false;
    bool deltaSemaphoreCreated = false;

    /* The first parameter of this demo function is not used. Shadows are specific
     * to AWS IoT, so this value is hardcoded to true whenever needed. */
    (void)awsIotMqttMode;

    /* Determine the length of the Thing Name. */
    if (pIdentifier != NULL)
    {
        thingNameLength = strlen(pIdentifier);

        IotLogInfo("thingNameLength is %d, Identifier %s",thingNameLength,pIdentifier);

        if (thingNameLength == 0)
        {
            IotLogError("The length of the Thing Name (identifier) must be nonzero.");

            status = EXIT_FAILURE;
        }
    }
    else
    {
        IotLogError("A Thing Name (identifier) must be provided for the Shadow demo.");

        status = EXIT_FAILURE;
    }

    /* Initialize the libraries required for this demo. */
    if (status == EXIT_SUCCESS)
    {
        status = _initializeDemo();
    }

    if (status == EXIT_SUCCESS)
    {
        /* Mark the libraries as initialized. */
        librariesInitialized = true;

        /* Establish a new MQTT connection. */
        status = _establishMqttConnection(pIdentifier,
                                          pNetworkServerInfo,
                                          pNetworkCredentialInfo,
                                          pNetworkInterface,
                                          &mqttConnection);
    }

    if (status == EXIT_SUCCESS)
    {
        /* Mark the MQTT connection as established. */
        connectionEstablished = true;

        /* Create the semaphore that synchronizes with the delta callback. */
        deltaSemaphoreCreated = IotSemaphore_Create( &deltaSemaphore, 0, 1 );

        if( deltaSemaphoreCreated == false )
        {
            status = EXIT_FAILURE;
        }
    }

    if( status == EXIT_SUCCESS )
    {
        /* Set the Shadow callbacks for this demo. */
        status = _setShadowCallbacks( &deltaSemaphore,
                                      mqttConnection,
                                      pIdentifier,
                                      thingNameLength );
    }

    if(status == EXIT_SUCCESS)
    {
        IotLogInfo("free heap size is %d bytes ",xPortGetFreeHeapSize());
        status = _thingShadowOperation( &deltaSemaphore,
                                        mqttConnection,
                                        pIdentifier,
                                        thingNameLength);
    }
    
    /* Disconnect the MQTT connection if it was established. */

    if (connectionEstablished == true)
    {
        IotMqtt_Disconnect(mqttConnection, 0);
    }

    /* Clean up libraries if they were initialized. */
    if (librariesInitialized == true)
    {
        _cleanupDemo();
    }
    /* Destroy the delta semaphore if it was created. */
    if( deltaSemaphoreCreated == true )
    {
        IotSemaphore_Destroy( &deltaSemaphore );
    }
    return status;
}