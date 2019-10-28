/*
 * Amazon FreeRTOS V201908.00
 * Copyright (C) 2019 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://aws.amazon.com/freertos
 * http://www.FreeRTOS.org
 */

#ifndef AWS_CLIENT_CREDENTIAL_KEYS_H
#define AWS_CLIENT_CREDENTIAL_KEYS_H

/*
 * @brief PEM-encoded client certificate.
 *
 * @todo If you are running one of the Amazon FreeRTOS demo projects, set this
 * to the certificate that will be used for TLS client authentication.
 *
 * @note Must include the PEM header and footer:
 * "-----BEGIN CERTIFICATE-----\n"\
 * "...base64 data...\n"\
 * "-----END CERTIFICATE-----\n"
 */
#define keyCLIENT_CERTIFICATE_PEM \
"-----BEGIN CERTIFICATE-----\n"\
"MIIDWjCCAkKgAwIBAgIVAJgOorCvScKVyNOFjSCeS+r8EMKhMA0GCSqGSIb3DQEB\n"\
"CwUAME0xSzBJBgNVBAsMQkFtYXpvbiBXZWIgU2VydmljZXMgTz1BbWF6b24uY29t\n"\
"IEluYy4gTD1TZWF0dGxlIFNUPVdhc2hpbmd0b24gQz1VUzAeFw0xOTEwMjQwODQy\n"\
"NDRaFw00OTEyMzEyMzU5NTlaMB4xHDAaBgNVBAMME0FXUyBJb1QgQ2VydGlmaWNh\n"\
"dGUwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQC3VxsaBtNVUNYXfT4o\n"\
"zcP/qgAwlmoyXK+UmjFxDsjW8WJUGL/lxEXgI7QRelPHl3/OUhSrJY/S3J2m7TD2\n"\
"jKkNvGN6yhh78s6X8pyt74ga3h85aSz3NX15bwctixJ5NDd8Lnz38Shoh7VWSsqX\n"\
"RmpJjiwlsbVaoBB6LcBTXH/fktkbIzlbU1sPe89t2JBe0q/v6K9WXRSGAilCc22G\n"\
"Q+sQmsUEBSJjF414T8jNKwoPn1yWz0N0NFUDs1/wQy7rJOu24I4YniqWzu25b9wR\n"\
"QP7eGvxRT4bI+M4Br3sY7hnFq+EViD3YlUWtFlElxssSnCcpNejPew1Jx1fgJtHc\n"\
"8/EFAgMBAAGjYDBeMB8GA1UdIwQYMBaAFLljzWE/TCWxW4z/uG6dxBIsh59+MB0G\n"\
"A1UdDgQWBBSxEYf5L4YZDXl8of+ughFcae9usDAMBgNVHRMBAf8EAjAAMA4GA1Ud\n"\
"DwEB/wQEAwIHgDANBgkqhkiG9w0BAQsFAAOCAQEAIz08Zpr645DqN/7E3NWxP5mk\n"\
"gYg/IC7gws3uelkpcnkl81KGumxr0ee137pos7Cfai7U2NstY8BsR8HfJ4dO3UiE\n"\
"fQGKRxEulrT/gOb7zDZjlZg1qVk6WXKwGECK+Tvq8MjQb5gymvpTjsI8W+I9Ijoz\n"\
"nf2R8yYMjyeULd4ALYGXhl3TYe1XT3pJDcQ2ho5A7q8jqSMON5Qk5k6OZZ2r3WQ2\n"\
"/H8VIpHrMIFpYG9gb7hdqkrXx70jxT2fRbztzHGKTLMR5ldlaElv2UsKTliheIt4\n"\
"UI7AXn1yI7hV8aWUMunxSe53zxW3EYJxwWdxIXwkvwGri8xqa19xNx3QUMqTEQ==\n"\
"-----END CERTIFICATE-----\n"

/*
 * @brief PEM-encoded issuer certificate for AWS IoT Just In Time Registration (JITR).
 *
 * @todo If you are using AWS IoT Just in Time Registration (JITR), set this to
 * the issuer (Certificate Authority) certificate of the client certificate above.
 *
 * @note This setting is required by JITR because the issuer is used by the AWS
 * IoT gateway for routing the device's initial request. (The device client
 * certificate must always be sent as well.) For more information about JITR, see:
 *  https://docs.aws.amazon.com/iot/latest/developerguide/jit-provisioning.html,
 *  https://aws.amazon.com/blogs/iot/just-in-time-registration-of-device-certificates-on-aws-iot/.
 *
 * If you're not using JITR, set below to NULL.
 *
 * Must include the PEM header and footer:
 * "-----BEGIN CERTIFICATE-----\n"\
 * "...base64 data...\n"\
 * "-----END CERTIFICATE-----\n"
 */
#define keyJITR_DEVICE_CERTIFICATE_AUTHORITY_PEM NULL

/*
 * @brief PEM-encoded client private key.
 *
 * @todo If you are running one of the Amazon FreeRTOS demo projects, set this
 * to the private key that will be used for TLS client authentication.
 *
 * @note Must include the PEM header and footer:
 * "-----BEGIN RSA PRIVATE KEY-----\n"\
 * "...base64 data...\n"\
 * "-----END RSA PRIVATE KEY-----\n"
 */
#define keyCLIENT_PRIVATE_KEY_PEM \
"-----BEGIN RSA PRIVATE KEY-----\n"\
"MIIEogIBAAKCAQEAt1cbGgbTVVDWF30+KM3D/6oAMJZqMlyvlJoxcQ7I1vFiVBi/\n"\
"5cRF4CO0EXpTx5d/zlIUqyWP0tydpu0w9oypDbxjesoYe/LOl/Kcre+IGt4fOWks\n"\
"9zV9eW8HLYsSeTQ3fC589/EoaIe1VkrKl0ZqSY4sJbG1WqAQei3AU1x/35LZGyM5\n"\
"W1NbD3vPbdiQXtKv7+ivVl0UhgIpQnNthkPrEJrFBAUiYxeNeE/IzSsKD59cls9D\n"\
"dDRVA7Nf8EMu6yTrtuCOGJ4qls7tuW/cEUD+3hr8UU+GyPjOAa97GO4ZxavhFYg9\n"\
"2JVFrRZRJcbLEpwnKTXoz3sNScdX4CbR3PPxBQIDAQABAoIBAGeB/gMLp3SbVsOc\n"\
"KePPh8BnsgO8Z+hW+6niA9fYiPsQ/HqOHokR7oFEpvzXG0Wr+rP5E0Isyq45b6g2\n"\
"cVZAw5DFs0Q3pOsVgh23E03/1c+VZ/t5rbFDubs9O8gypBNZr5mkhVlWBYI+W9qE\n"\
"DyyozIkKg1Ikqkan56WS/uoRZwBJaoAg0ioQbzgcEGc/EVrzzmmIjpSW8iJkD8aT\n"\
"1xzBin1HOUFEeFRQiLb5+9H6Q7pdiLmROt1wv42BOwCoHr/65WAgbD8XYkNIq5Hf\n"\
"7u4kDNqlUtXoQ3TRAIw5xKMojstJPijug0HCmJuqK3joTq3DOLbTDuD2r/wUgZiD\n"\
"ZVqPEmECgYEA6fQqx6616yevSIISTiyYbU28QX75X85F0zqU/JX37OSdOsdygTqJ\n"\
"LIn/HpcD/LWdQiVfN+EywQTLXZYqRDkol5SWkMepEuq/6ZLDGvtQtaqcyJcJscjv\n"\
"o0BM28ds+x0LeermCmB49pt7/n/whZM0lFMi2XI51f4qDe8hvF8JHt8CgYEAyJ3z\n"\
"8dkZeHstRxP2k35MyyX45WuG3OS/so9YEGba3COQrI0RwrInVcDaXBVXqqDLOo2Z\n"\
"FdLNIDZ5P2Q0fGEbH07OhpdIqeRUp/k1BhEaOLcopXPKAGpyE3i/jSEJNxjvKHA7\n"\
"t3ryx2olUOkeOykLF437G9iJ/xg3n94OHviZwJsCgYBhyKwYye0/XFbm93avNfR/\n"\
"uv30Yh3Y7V5B6zpaWFRHr0YIS1SQruamtwbqwjPobABOpVAnCvkrs9VMSHdiDIHq\n"\
"2TAVU15Xicas2aqynFgfli/xpy9mH5YJpdZ6BeCHFJy3WHbN34Svjk1FEjR1oBFZ\n"\
"7WWjfRqvVow9RrXTBeXKAQKBgA/9xzw3+ffImctYpAcNwkFPPY/MCmwBL+aj1nk4\n"\
"Oanii921QP0sBkanoMBGUz6eopSARPugWgl/ThrneeeQNgFA3uWLmZcxRC4/hyJ8\n"\
"qoJq1yqZGDkAQyeaMGqnf3yM2EN47smpW0DaDS9t5aMBmoi+II4PrLBQ1d5AbQ+6\n"\
"8UaRAoGAKip1P5D8ug4iVS4ITgf8SQ7tn1LXyuEG3qauBTU0DGNvPamGjC3hPpUj\n"\
"nHEgq96mzesuoH4sfU8YquQTcOss43yRzmfx8y14jQIMCl52zTgMMbwzDvVqemgE\n"\
"41h6akA3YcNy01m7D/pEkLh1nqRK9qxADzkzPV9VdZwJN6SwR+0=\n"\
"-----END RSA PRIVATE KEY-----\n"

#endif /* AWS_CLIENT_CREDENTIAL_KEYS_H */
