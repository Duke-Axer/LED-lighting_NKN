#include <WiFi.h>
#include <SPIFFS.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>

const char* main_ssid = "TP-LINK_Dom";         // Główna sieć Wi-Fi
const char* main_password = "***:D";
const char* ap_ssid = "ESP_Master";            // Nowa sieć Wi-Fi
const char* ap_password = "haslo111";

WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

int varOprawa1 = 100;
int varOprawa2 = 100;

void setup() {
  Serial.begin(115200);
  if (!SPIFFS.begin(true)) {
    Serial.println("Nie udało się zainicjować SPIFFS");
    return;
  }

  // Połączenie z główną siecią
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(main_ssid, main_password);
  Serial.print("Łączenie z główną siecią...");
  if (WiFi.status() != WL_CONNECTED) {
    delay(2000);
  }
  Serial.println("\nPołączono z główną siecią!");
  // Ustawienie nowej sieci Wi-Fi
  WiFi.softAP(ap_ssid, ap_password);
  Serial.println("Nowa sieć Wi-Fi utworzona!");

  // Obsługa żądań na stronie głównej
  server.on("/", HTTP_GET, []() {
    File file = SPIFFS.open("/index.html", "r");
    if (!file) {
      server.send(404, "text/plain", "404: Not Found");
      return;
    }
    server.streamFile(file, "text/html");
    file.close();
  });

  // Obsługa żądania GET do wysłania początkowych danych
//  server.on("/start-serwer", HTTP_GET, []() {
//    StaticJsonDocument<200> jsonDoc;
//    jsonDoc["varOprawa1"] = varOprawa1;
//    jsonDoc["varOprawa2"] = varOprawa2;
//    jsonDoc["IP_STA"] = WiFi.localIP().toString();
//    jsonDoc["IP_AP"] = WiFi.softAPIP().toString();
//
//    String jsonResponse;
//    serializeJson(jsonDoc, jsonResponse);
//    server.send(200, "application/json", jsonResponse);
//  });

  //    server.on("/push", HTTP_POST, []() {
  //      if (server.hasArg("oprawa1")) {
  //        oprawa1 = server.arg("oprawa1");
  //        Serial.print("Ustawiono wartość suwaka 1: ");
  //        Serial.println(oprawa1);
  //      }
  //      if (server.hasArg("oprawa2")) {
  //        oprawa2 = server.arg("oprawa2");
  //        Serial.print("Ustawiono wartość suwaka 2: ");
  //        Serial.println(oprawa2);
  //      }
  //    }

  // Start serwera
  server.begin();
  Serial.println("Serwer uruchomiony!");
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  Serial.print("Adres IP nowej sieci: ");
  Serial.println(WiFi.softAPIP());
  Serial.print("Adres IP w głównej sieci: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  server.handleClient(); // Obsługuje klienta
  webSocket.loop();      // Obsługuje WebSocket


}

// Funkcja obsługująca zdarzenia WebSocket
void webSocketEvent(uint8_t clientNum, WStype_t type, uint8_t *payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED: // Klient odłączony
      Serial.printf("Klient %u odłączony\n", clientNum);
      break;

    case WStype_CONNECTED: { // Klient połączony
        IPAddress ip = webSocket.remoteIP(clientNum);
        Serial.printf("Klient %u połączony z IP: %s\n", clientNum, ip.toString().c_str());
        StaticJsonDocument<200> jsonDoc;
        jsonDoc["varOprawa1"] = varOprawa1;
        jsonDoc["varOprawa2"] = varOprawa2;
        jsonDoc["IP_STA"] = WiFi.localIP().toString();
        jsonDoc["IP_AP"] = WiFi.softAPIP().toString();

        String jsonResponse;
        serializeJson(jsonDoc, jsonResponse);
        webSocket.sendTXT(clientNum, jsonResponse);
        break;
      }

    case WStype_TEXT: // Otrzymano tekst
      String message = String((char*)payload);
      // Odpowiedź do wszystich
      for (uint8_t i = 0; i < webSocket.connectedClients(); i++) {
        if (i != clientNum) { // Pomija klienta, który wysłał wiadomość
          webSocket.sendTXT(i, message);
        }
      }
      StaticJsonDocument<200> jsonDoc;
      deserializeJson(jsonDoc, message);
      Serial.printf("Odebrano od klienta %u: %s\n", clientNum, message.c_str());
      varOprawa1 = jsonDoc["varOprawa1"];
      varOprawa2 = jsonDoc["varOprawa2"];
      if (jsonDoc.containsKey("resetFixture")) {
        String resetValue = jsonDoc["resetFixture"].as<String>();
        Serial.print("Wartość resetFixture: ");
        Serial.println(resetValue);
      }

      break;
  }
}
