#include "webServer.h"

#include <ArduinoJson.h>

#include "SPIFFS.h"
#include "fileSystem.h"
#include "ledStrip.h"
#include "mbdtls.h"

#define RESPONSE_DELAY 500  // delay before reboot

AsyncWebServer server(80);
String fillPlaceholders(const String &var) {
    Serial.println(var);
    if (var == "LOGIN") {
        return personMail;
    }
    if (var == "TOKEN") {
        return token;
    }
    if (var == "HOSTNAME") {
        return host;
    }
    if (var == "BRPORT") {
        return brPort;
    }
    if (var == "PRODUCTID") {
        return productId;
    }
    if (var == "DEVICEID") {
        return deviceId;
    }

    if (var == "FIRMWARE") {
        return firmwareName;
    }
    return String();
}

void onRequestWithAuth(AsyncWebServerRequest *request, ArRequestHandlerFunction onRequest) {
    if (!request->authenticate(http_username, webAuthPass.c_str())) return request->requestAuthentication();

    onRequest(request);
}

void setupWebServer() {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        onRequestWithAuth(request, [](AsyncWebServerRequest *request) {
            request->send(SPIFFS, "/index.html", String(), false, fillPlaceholders);
        });
    });

    server.on("/index.html", HTTP_GET, [](AsyncWebServerRequest *request) {
        onRequestWithAuth(request, [](AsyncWebServerRequest *request) {
            request->send(SPIFFS, "/index.html", String(), false, fillPlaceholders);
        });
    });

    server.on("/header.html", HTTP_GET, [](AsyncWebServerRequest *request) {
        onRequestWithAuth(request, [](AsyncWebServerRequest *request) {
            request->send(SPIFFS, "/header.html", String(), false, fillPlaceholders);
        });
    });

    server.on("/wifi.html", HTTP_GET, [](AsyncWebServerRequest *request) {
        onRequestWithAuth(request, [](AsyncWebServerRequest *request) {
            request->send(SPIFFS, "/wifi.html", String(), false, fillPlaceholders);
        });
    });

    server.on("/settings.html", HTTP_GET, [](AsyncWebServerRequest *request) {
        onRequestWithAuth(request, [](AsyncWebServerRequest *request) {
            request->send(SPIFFS, "/settings.html", String(), false, fillPlaceholders);
        });
    });

    server.on("/system.html", HTTP_GET, [](AsyncWebServerRequest *request) {
        onRequestWithAuth(request, [](AsyncWebServerRequest *request) {
            request->send(SPIFFS, "/system.html", String(), false, fillPlaceholders);
        });
    });

    server.on("/static/favicon.png", HTTP_GET,
              [](AsyncWebServerRequest *request) { request->send(SPIFFS, "/static/favicon.png", "image/png"); });

    server.on("/static/logo.svg", HTTP_GET,
              [](AsyncWebServerRequest *request) { request->send(SPIFFS, "/static/logo.svg", "image/svg+xml"); });

    server.on("/styles.css", HTTP_GET,
              [](AsyncWebServerRequest *request) { request->send(SPIFFS, "/styles.css", "text/css"); });

    server.on("/healthcheck", HTTP_GET, [](AsyncWebServerRequest *request) { request->send(200, "text/html", "OK"); });

    server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request) {
        onRequestWithAuth(request, [](AsyncWebServerRequest *request) {
            request->send(200, "text/plain", "OK");
            delay(RESPONSE_DELAY);
            ESP.restart();
        });
    });

    server.on("/resetdefault", HTTP_GET, [](AsyncWebServerRequest *request) {
        onRequestWithAuth(request, [](AsyncWebServerRequest *request) {
            if (!eraseFlash()) {
                request->send(500, "text/plain", "Server error");
                return;
            }
            request->send(200, "text/plain", "OK");
            delay(RESPONSE_DELAY);
            ESP.restart();
        });
    });

    server.on("/newauthpass", HTTP_GET, [](AsyncWebServerRequest *request) {
        onRequestWithAuth(request, [](AsyncWebServerRequest *request) {
            if (!request->hasParam("newpass")) {
                request->send(400);
                return;
            }

            webAuthPass = request->getParam("newpass")->value();
            if (!saveConfig()) {
                request->send(500, "text/plain", "Server error");
                return;
            }

            request->send(200, "text/plain", "OK");
            delay(RESPONSE_DELAY);
            ESP.restart();
        });
    });

    server.on("/setwifi", HTTP_GET, [](AsyncWebServerRequest *request) {
        onRequestWithAuth(request, [](AsyncWebServerRequest *request) {
            if (!request->hasParam("ssid") || !request->hasParam("pass")) {
                request->send(400);
                return;
            }

            ssidName = request->getParam("ssid")->value();
            ssidPassword = request->getParam("pass")->value();

            if (!saveConfig()) {
                request->send(500, "text/plain", "Server error");
                return;
            }
            request->send(200, "text/plain", "OK");
            delay(RESPONSE_DELAY);
            ESP.restart();
        });
    });

    server.on("/scan", HTTP_GET, [](AsyncWebServerRequest *request) {
        onRequestWithAuth(request, [](AsyncWebServerRequest *request) {
            DynamicJsonDocument doc(1024);
            int n = WiFi.scanComplete();
            if (n == WIFI_SCAN_FAILED) {
                WiFi.scanNetworks(true);
            } else if (n) {
                for (int i = 0; i < n; ++i) {
                    doc[WiFi.SSID(i)] = String(WiFi.encryptionType(i));
                }
                WiFi.scanDelete();
                if (WiFi.scanComplete() == WIFI_SCAN_FAILED) {
                    WiFi.scanNetworks(true);
                }
            }
            String response;
            serializeJson(doc, response);
            request->send(200, "application/json", response);
        });
    });

    server.on("/connectedwifi", HTTP_GET, [](AsyncWebServerRequest *request) {
        onRequestWithAuth(request, [](AsyncWebServerRequest *request) {
            request->send(200, "text/plain", WiFi.status() == WL_CONNECTED ? ssidName : "NULL");
        });
    });

    server.on("/setcredentials", HTTP_GET, [](AsyncWebServerRequest *request) {
        onRequestWithAuth(request, [](AsyncWebServerRequest *request) {
            if (!request->hasParam("mail") || !request->hasParam("token") || !request->hasParam("hostname") ||
                !request->hasParam("brokerPort") || !request->hasParam("productId") || !request->hasParam("deviceId")) {
                request->send(400, "text/plain", "Incorrect data");
                return;
            }

            personMail = request->getParam("mail")->value();
            token = request->getParam("token")->value();
            host = request->getParam("hostname")->value();
            brPort = request->getParam("brokerPort")->value();
            productId = request->getParam("productId")->value();
            deviceId = request->getParam("deviceId")->value();
            personId = sha256(personMail);

            Serial.println(personMail);
            Serial.println(personId);
            Serial.println(token);
            Serial.println(host);
            Serial.println(brPort);
            Serial.println(productId);
            Serial.println(deviceId);
            if (!saveConfig()) {
                request->send(500, "text/plain", "Server error");
                return;
            }

            request->send(200, "text/plain", "OK");
            delay(RESPONSE_DELAY);
            ESP.restart();
        });
    });

    server.on("/pair", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (!request->hasParam("ssid") || !request->hasParam("psk") || !request->hasParam("wsp") ||
            !request->hasParam("token") || !request->hasParam("host") || !request->hasParam("brport")) {
            request->send(400, "text/plain", "Incorrect data");
            return;
        }

        ssidName = request->getParam("ssid")->value();
        ssidPassword = request->getParam("psk")->value();
        personMail = request->getParam("wsp")->value();
        token = request->getParam("token")->value();
        host = request->getParam("host")->value();
        brPort = request->getParam("brport")->value();

        String devId = WiFi.macAddress();
        devId.toLowerCase();
        devId.replace(":", "-");
        deviceId = devId;
        personId = sha256(personMail);
        Serial.println("WebServer update:");
        Serial.println("SSID_Name = " + ssidName);
        Serial.println("SSID_Password = " + ssidPassword);
        Serial.println("personMail = " + personMail);
        Serial.println("personId = " + personId);
        Serial.println("token = " + token);
        Serial.println("host = " + host);
        Serial.println("brport = " + brPort);
        Serial.println("deviceId = " + deviceId);
        if (ssidName == "") {
            request->send(400, "text/plain", "Wifi name is NULL");
            return;
        }
        if (!saveConfig()) {
            request->send(500, "text/plain", "Server error");
            return;
        }

        request->send(200, "text/plain", "OK");
        delay(RESPONSE_DELAY);
        ESP.restart();
    });

    server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request) {
        onRequestWithAuth(request, [](AsyncWebServerRequest *request) {
            if (!request->hasParam("state") || !request->hasParam("brightness") || !request->hasParam("mode")) {
                request->send(400);
                return;
            }
            uint8_t brightness = request->getParam("brightness")->value().toInt();
            if (ls.brightness != brightness) {
                ls.brightness = brightness;
                newLsState = NEW_BRIGHTNESS;
                checkNewState();
            }
            ls.mode = request->getParam("mode")->value().toInt();
            ls.state = request->getParam("state")->value().toInt();
            ls.red = request->getParam("r")->value().toInt();
            ls.green = request->getParam("g")->value().toInt();
            ls.blue = request->getParam("b")->value().toInt();

            newLsState = NEW_MODE;
            newLsMqttData = true;
            checkNewState();
            request->send(200, "text/plain", "OK");
        });
    });

    server.on("/settings", HTTP_GET, [](AsyncWebServerRequest *request) {
        onRequestWithAuth(request, [](AsyncWebServerRequest *request) {
            StaticJsonDocument<256> doc;
            char colorBuffer[12];  // length of RGB mess
            snprintf(colorBuffer, sizeof(colorBuffer), "%d,%d,%d", ls.red, ls.green, ls.blue);

            doc["data"]["state"] = !!ls.state;
            doc["data"]["mode"] = lsModes.at(ls.mode);
            doc["data"]["color"] = colorBuffer;
            doc["data"]["brightness"] = ls.brightness;

            std::string modeFormatValue = "";
            for (int i = 0; i < lsModes.size(); i++) {
                modeFormatValue += lsModes.at(i);
                if (lsModes.size() - i - 1) modeFormatValue += ",";
            }

            doc["data"]["states"] = modeFormatValue;
            String response;
            serializeJson(doc, response);
            request->send(200, "application/json", response);
        });
    });

    server.on(
        "/firmware/upload", HTTP_POST,
        [](AsyncWebServerRequest *request) {
            onRequestWithAuth(request, [](AsyncWebServerRequest *request) {
                request->send((Update.hasError()) ? 500 : 200);
                delay(500);
                ESP.restart();
            });
        },

        onFirmwareUpload);

    server.onNotFound([](AsyncWebServerRequest *request) { request->send(404); });

    server.begin();
}

void onFirmwareUpload(AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len,
                      bool final) {
    if (!index) {
        uint32_t freeSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
        if (!Update.begin(freeSpace)) {
            Update.printError(Serial);
        }
    }
    if (!Update.hasError()) {
        if (Update.write(data, len) != len) {
            Update.printError(Serial);
        }
    }

    if (final) {
        if (!Update.end(true)) {
            Update.printError(Serial);
        } else {
            Serial.println("Update complete");
        }
    }
}

String sha256(String rec_data) {
    const char *payload = rec_data.c_str();
    byte shaResult[32];

    mbedtls_md_context_t ctx;
    mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;

    const size_t payloadLength = strlen(payload);

    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 0);
    mbedtls_md_starts(&ctx);
    mbedtls_md_update(&ctx, (const unsigned char *)payload, payloadLength);
    mbedtls_md_finish(&ctx, shaResult);
    mbedtls_md_free(&ctx);

    Serial.print("Hash: ");
    String result = "";
    for (int i = 0; i < sizeof(shaResult); i++) {
        char str[3];
        snprintf(str, sizeof(shaResult), "%02x", static_cast<int>(shaResult[i]));
        Serial.print(str);
        result = result + str;
    }
    return result;
}
