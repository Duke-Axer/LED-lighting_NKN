#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>

#include "BluetoothSerial.h"


const char* ssid     = "TP-LINK_Dom";
const char* password = "Nekran924";

String wartosc = "NULL!";

AsyncWebServer server(80);







unsigned long previousMillis;
unsigned long timeForMessage = millis(); 
String message = "";


bool MasterConnected; 
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)                           
#error Bluetooth is not enabled! Please run make menuconfig to and enable it
#endif
#if !defined(CONFIG_BT_SPP_ENABLED)
#error Serial Bluetooth not available or not enabled. It is only available for the ESP32 chip.
#endif
BluetoothSerial SerialBT;  
void Bt_Status (esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {

  if (event == ESP_SPP_SRV_OPEN_EVT) {
    Serial.println ("Client Connected");
    SerialBT.print("Client Connected");
    MasterConnected = true;
  }
  else if (event == ESP_SPP_CLOSE_EVT ) {
    Serial.println ("Client Disconnected");
    MasterConnected = false;
  }
}

void setup() {
    Serial.begin(115200);
    Serial.println("Łączenie z WiFi...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("\nPołączono z WiFi!");

    // Inicjalizacja SPIFFS
    if (!SPIFFS.begin(true)) {
        Serial.println("Błąd inicjalizacji SPIFFS");
        return;
    }
    Serial.println("SPIFFS zainicjalizowane!");

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/index.html", "text/html");
    });

    // Ustawienie routingu dla pobierania parametru
    server.on("/pobierz-parametr", HTTP_GET, [](AsyncWebServerRequest *request){
        // Tutaj możesz wykonać operacje do pobrania parametru
        
        request->send(200, "text/plain", wartosc);
        Serial.print("Wartość parametru: ");
        Serial.println(wartosc);
    });

    // Ustawienie routingu dla ustawiania parametru
    server.on("/ustaw-parametr", HTTP_POST, [](AsyncWebServerRequest *request){
        // Odczytaj przekazaną wartość parametru
        if (request->hasParam("parametr", true)) {
            String newValue = request->getParam("parametr", true)->value();
            wartosc = newValue;
            Serial.print("Nowa wartość parametru: ");
            Serial.println(wartosc);
            request->send(200, "text/plain", "Wartość parametru została zaktualizowana");
        } else {
            request->send(400, "text/plain", "Nieprawidłowe żądanie");
        }
    });

    // Start serwera
    server.begin();
    Serial.println("Serwer uruchomiony!");


  
  SerialBT.begin("ESP32");
  SerialBT.register_callback (Bt_Status);
  Serial.printf("Serwer jest uruchomiony!\nMożna sparować z BLUETOOTH!\n");
}

void loop() {
    if (Serial.available()) {
        String input = Serial.readStringUntil('\n');
        wartosc = input;
        Serial.print("Zaktualizowana wartość parametru: ");
        Serial.println(wartosc);
    }

    if (!MasterConnected) {
    if (millis() - previousMillis >= 500) { 
     Serial.println ("!MasterConnected");    
    }
    
  }

if (SerialBT.available()) {
    char receivedChar = SerialBT.read();
    if (message == ""){
      timeForMessage = millis(); 
    }
    message += receivedChar;
  }

if (millis()- timeForMessage >= 500 && message != ""){
      Serial.println(millis()- timeForMessage);
      Serial.println("otrzymanie: " + message);
      if (message.startsWith("0")){
        Serial.print("Poloczenie z telefonem");
      }else if (message.startsWith("StartLED")){
        Serial.println("Poloczenie jakims LED" + message + message[8]);
      }else {
        Serial.print("wiadmosc" + message);
        Serial.print("to nie: StartLED");
        SerialBT.print("Start XX");
      }
      message = "";
      timeForMessage = millis();
    }
}
