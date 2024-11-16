#include <WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>  // Biblioteka ArduinoJson
#include "BluetoothSerial.h"
#include <SPIFFS.h>
String ssid = "0";         // Nazwa nowej sieci Wi-Fi
String password = "0";     // Hasło do nowej sieci Wi-Fi
String serverHost = "0";   // Adres IP ESP serwera
uint16_t serverPort = 81;  // Port serwera WebSocket
char* serverPath = "/";    // Ścieżka serwera
String lightFixture = "0";
bool resetFixture = false;  // Resetowanie klienta

WebSocketsClient webSocket;
BluetoothSerial SerialBT;

unsigned long previousMillis = 0;  // Przechowuje ostatni czas
unsigned long interval = 3;        // Ustawienia interwału w milisekundach (1 sekunda)
int dutyCycle = 0;                 // Przechowuje wartość cyklu pracy
bool increasing = true;            // Flaga do kontrolowania kierunku zmian jasności
unsigned long currentMillis;
String messageBTMenu = "";
String messageBT = "";
bool btON = false;

const int pinLED = 16;

void readFile() {
  File file = SPIFFS.open("/config.txt", "r");
  String line;
  int lineNumber = 0;

  while (file.available()) {
    line = file.readStringUntil('\n');  // Odczytaj linię
    line.trim();
    if (lineNumber == 0) {
      ssid = line;
    } else if (lineNumber == 1) {
      password = line;
    } else if (lineNumber == 2) {
      serverHost = line;
    } else if (lineNumber == 3) {
      lightFixture = line;
    }
    lineNumber++;
  }
  file.close();  // Zamyka plik
}
void saveFile() {
  File file = SPIFFS.open("/config.txt", "w");
  file.println(ssid);
  file.println(password);
  file.println(serverHost);
  file.println(lightFixture);
  file.close();Fu
}
void flashing() {
  currentMillis = millis();

  // Sprawdź, czy minął czas na zmianę jasności
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    // Zmiana jasności diody LED
    analogWrite(pinLED, dutyCycle);

    if (increasing) {
      dutyCycle++;
      if (dutyCycle >= 255) {
        increasing = false;  // Zmień kierunek na zmniejszanie
      }
    } else {
      dutyCycle--;
      if (dutyCycle <= 0) {
        increasing = true;  // Zmień kierunek na zwiększanie
      }
    }
  }
}
void menu() {
  SerialBT.println("Menu (zmień):");
  SerialBT.println("1. Nazwa: " + String(lightFixture));
  SerialBT.println("2. Serwer: " + String(ssid));
  SerialBT.println("3. Hasło: " + String(password));
  SerialBT.println("4. Host: " + String(serverHost));
}
String readBT() {
  String messageBT = "";  // Inicjalizuj pusty String
  if (SerialBT.available()) {
    messageBT = SerialBT.readString();  // Odbierz wiadomość
    messageBT.trim();
    Serial.println(messageBT);  // Wyświetl wiadomość w monitorze szeregowym
  }
  return messageBT;  // Zwróć odebraną wiadomość
}
void bluetoothActive() {
  flashing();
  Serial.println("Próba połączenia z Bluetooth");
  if (btON == false) {
    SerialBT.begin(lightFixture);
    btON = true;
  }
  delay(1000);
  if (SerialBT.connected()) {
    menu();
    messageBTMenu = "x";
    while (messageBTMenu != "0") {
      flashing();
      messageBT = "x";
      messageBTMenu = readBT();
      if (messageBTMenu == "1") {  // Obsługa menu
        SerialBT.println("1. Nazwa: " + String(lightFixture));
        SerialBT.println("Wybrano zmianę nazwy, 0 cofnij");
        while (messageBT != "0") {
          messageBT = readBT();
          if (messageBT.length() > 1) {
            lightFixture = messageBT;
            SerialBT.println("Zmieniono Nazwę na: " + String(lightFixture));
            menu();
            break;
          } else if (messageBT == "0") {
            menu();
            break;
          }
          if (!SerialBT.connected()) {
            break;
          }
        }
      } else if (messageBTMenu == "2") {
        SerialBT.println("2. Serwer: " + String(ssid));
        SerialBT.println("Wybrano zmianę Serwera, 0 cofnij");
        while (messageBT != "0") {
          messageBT = readBT();
          if (messageBT.length() > 1) {
            ssid = messageBT;
            SerialBT.println("Zmieniono Serwer na: " + String(ssid));
            menu();
            break;
          } else if (messageBT == "0") {
            menu();
            break;
          }
          if (!SerialBT.connected()) {
            break;
          }
        }
      } else if (messageBTMenu == "3") {
        SerialBT.println("2. Hasło: " + String(password));
        SerialBT.println("Wybrano zmianę Hasła, 0 cofnij");
        while (messageBT != "0") {
          messageBT = readBT();
          if (messageBT.length() > 1) {
            password = messageBT;
            SerialBT.println("Zmieniono Hasło na: " + String(password));
            menu();
            break;
          } else if (messageBT == "0") {
            menu();
            break;
          }
          if (!SerialBT.connected()) {
            break;
          }
        }
      } else if (messageBTMenu == "4") {
        SerialBT.println("2. Host: " + String(serverHost));
        SerialBT.println("Wybrano zmianę Hosta, 0 cofnij");
        while (messageBT != "0") {
          messageBT = readBT();
          if (messageBT.length() > 1) {
            serverHost = messageBT;
            SerialBT.println("Zmieniono Host na: " + String(serverHost));
            menu();
            break;
          } else if (messageBT == "0") {
            menu();
            break;
          }
          if (!SerialBT.connected()) {
            break;
          }
        }
      }

      if (!SerialBT.connected()) {  //Zerwane połącenie Bluetooth
        break;
      }
    }
    saveFile();
    resetFixture = false;
    SerialBT.end();
    WiFi.disconnect(true);
    btON = false;
  }
}
void wifiManager() {
  if (resetFixture == false) {
    WiFi.begin(ssid.c_str(), password.c_str());
    delay(1000);
    Serial.println(WiFi.status());
    if (WiFi.status() == WL_DISCONNECTED) {
      Serial.println("Rozłączono z siecią WiFi");
      Serial.println("SSID: " + ssid);
      Serial.println("Hasło: " + password);
      bluetoothActive();
    } else if (WiFi.status() == WL_NO_SSID_AVAIL) {
      Serial.println("Podana sieć WiFi nie istnieje");
      Serial.println("SSID: " + ssid);
      Serial.println("Hasło: " + password);
      bluetoothActive();
    } else if (WiFi.status() == WL_CONNECT_FAILED) {
      Serial.println("Nie poprawne hasło");
      Serial.println("SSID: " + ssid);
      Serial.println("Hasło: " + password);
      bluetoothActive();
    } else if (WiFi.status() == WL_IDLE_STATUS) {
      Serial.println("Wifi jest włączone ale nie szuka sieci");
    } else {
      Serial.println("Łączenie z siecią Wi-Fi...");
      delay(1000);
    }
  } else {
    Serial.println("Próba łączenia tylko z Bluetooth");
    bluetoothActive();
  }
}

void setup() {
  pinMode(pinLED, OUTPUT);
  Serial.begin(115200);
  // Inicjalizacja SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("Nie można zainicjalizować SPIFFS");
    return;  // Zatrzymaj dalsze wykonywanie, jeśli SPIFFS nie działa
  }
  readFile();
  Serial.println(password);
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);  // Ustawienie trybu Station Mode
  Serial.println("Ustawiono tryb Station Mode");
  WiFi.setScanTimeout(5000);  // Ustaw czas na 5 sekund

  // Inicjalizacja WebSocket
  webSocket.begin(serverHost.c_str(), serverPort, serverPath);
  webSocket.onEvent(webSocketEvent);
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    webSocket.loop();
  }
  if (WiFi.status() != WL_CONNECTED) {  // Obsługuje połączenie WiFi
    Serial.println("Brak połączenia z serverem");
    wifiManager();
    delay(2000);
  }
}

// Funkcja obsługująca zdarzenia WebSocket
void webSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.println("Rozłączono z serwerem WebSocket");
      webSocket.disconnect();
      //WiFi.disconnect();
      break;
    case WStype_CONNECTED:
      Serial.println("Połączono z serwerem WebSocket");
      if (btON == true) {
        SerialBT.end();
        btON = false;
      }
      break;
    case WStype_TEXT:
      {  // Odbieranie wiadomości tekstowej (w formacie JSON)
        String jsonString = String((char*)payload).substring(0, length);
        Serial.println("Odebrano dane JSON: " + jsonString);

        // Parsowanie JSON
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, jsonString);

        if (error) {
          Serial.print("Błąd parsowania JSON: ");
          Serial.println(error.c_str());
          return;
        }

        // Dostęp do wartości z JSON
        if (doc.containsKey(lightFixture)) {
          int intensity = doc[lightFixture].as<int>() * 255 / 100;  // Skala 0-255 dla PWM
          Serial.print("Odczytane natężenie oświetlenia: ");
          Serial.println(intensity);
          // Ustawianie jasności LED
          analogWrite(pinLED, intensity);
        }
        if (doc.containsKey("resetFixture")) {
          String resetValue = doc["resetFixture"].as<String>();  // Użyj as<String>() aby uzyskać wartość
          Serial.print("Wartość resetFixture: ");
          Serial.println(resetValue);  // Wydrukuj wartość

          // Sprawdź, czy wartość jest równa lightFixture
          if (lightFixture == resetValue) {
            Serial.println("Reset klienta");
            webSocket.disconnect();
            WiFi.disconnect(true);
            resetFixture = true;
          }
        }
        break;
      }
    case WStype_BIN:
      Serial.println("Odebrano binarną wiadomość");
      break;
  }
}
