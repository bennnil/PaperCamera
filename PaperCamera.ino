/******************************************************************************
 * ESP32-CAM - KI-Analyse mit Gemini, SD-Speicherung und Thermodruck
 * - Verwendet die korrekten Kamera-Pins 
 * - Integriert verbesserte Kamera-Konfiguration und Bild-Optimierung.
 * - Verlegt Peripherie auf endgültige, konfliktfreie Pins.
 ******************************************************************************/

// === Bibliotheken einbinden ===
#include "esp_camera.h"
#include "WiFi.h"
#include "SD_MMC.h"
#include "FS.h"
#include "Adafruit_Thermal.h"
#include "HTTPClient.h"
#include "ArduinoJson.h"
#include <Adafruit_NeoPixel.h>

// === Konfigurations-Variablen ===
String WIFI_SSID = "";
String WIFI_PASS = "";
String API_KEY   = "";
String API_URL   = "";

// === Finale Pin-Definitionen für Peripherie ===
#define BUTTON_PIN 1
#define PRINTER_RX_PIN 2
#define PRINTER_TX_PIN 3
#define ZEICHEN_PRO_ZEILE 32

// === NEU: LED Konfiguration ===
#define LED_PIN 21       // Hier Pin 21
#define NUM_LEDS 1
Adafruit_NeoPixel pixels(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// === Pin-Definitionen  ===
#define PWDN_GPIO_NUM -1
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 15
#define SIOD_GPIO_NUM 4
#define SIOC_GPIO_NUM 5
#define Y2_GPIO_NUM 11
#define Y3_GPIO_NUM 9
#define Y4_GPIO_NUM 8
#define Y5_GPIO_NUM 10
#define Y6_GPIO_NUM 12
#define Y7_GPIO_NUM 18
#define Y8_GPIO_NUM 17
#define Y9_GPIO_NUM 16
#define VSYNC_GPIO_NUM 6
#define HREF_GPIO_NUM 7
#define PCLK_GPIO_NUM 13

// === Globale Objekte und Variablen ===
HardwareSerial mySerial(2); 
Adafruit_Thermal printer(&mySerial);
String promptFromFile = "";

// === NEU: Hilfsfunktion für LED Farben ===
void setStatusLED(uint8_t r, uint8_t g, uint8_t b) {
  pixels.setPixelColor(0, pixels.Color(r, g, b));
  pixels.show();
}

// === Drucker-Funktionen ===
String replace_all(String text){
  text.replace("Ä","\x5B"); text.replace("ä","\x7B");
  text.replace("Ö","\x5C"); text.replace("ö","\x7C");
  text.replace("Ü","\x5D"); text.replace("ü","\x7D");
  text.replace("ß","\x7E");
  return text;
}
String getValue(String data, char separator, int index) {
  int found = 0; int strIndex[] = {0, -1}; int maxIndex = data.length() - 1;
  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++; strIndex[0] = strIndex[1] + 1; strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}
void drucke(const char* text) {
    String originalText = String(text); String currentLine = ""; int wordIndex = 0; String nextWord = "";
    
    do {
        nextWord = getValue(originalText, ' ', wordIndex++);
        if (nextWord.length() > 0) {
            if (currentLine.length() + nextWord.length() + 1 > ZEICHEN_PRO_ZEILE) {
                printer.println(replace_all(currentLine)); currentLine = "";
            }
            currentLine += nextWord + " ";
        }
    } while (nextWord.length() > 0);
    if (currentLine.length() > 0) { printer.println(replace_all(currentLine)); }
    printer.feed(3);
}

// === Setup-Funktion ===
void setup() {
  Serial.begin(115200);

  pixels.begin();
  pixels.setBrightness(50)
  setStatusLED(255, 165, 0);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // SD-Karte mit korrekten Pins und Befehl initialisieren
  SD_MMC.setPins(39, 38, 40); // CLK, CMD, D0
  if (!SD_MMC.begin("/sdcard", true, true, SDMMC_FREQ_DEFAULT, 5)){
    Serial.println("SD-Karten-Mount fehlgeschlagen! STOP."); 
    setStatusLED(255, 0, 0);
    while(1);
  }
  Serial.println("Integrierte SD-Karte über SD_MMC initialisiert.");

  // Lade Konfigurationsdaten aus config.txt
  File configFile = SD_MMC.open("/config.txt");
  if (configFile) {
    WIFI_SSID = configFile.readStringUntil('\n');
    WIFI_PASS = configFile.readStringUntil('\n');
    API_KEY   = configFile.readStringUntil('\n');
    API_URL   = configFile.readStringUntil('\n');
    configFile.close();
    // Leerzeichen entfernen
    WIFI_SSID.trim(); 
    WIFI_PASS.trim(); 
    API_KEY.trim();
    API_URL.trim();

    // Fallback, falls Zeile 4 leer ist oder vergessen wurde
    if (API_URL.length() == 0) {
      Serial.println("Warnung: Keine URL in config.txt gefunden. Nutze Standard.");
      API_URL = "https://generativelanguage.googleapis.com/v1beta/models/gemini-2.5-flash:generateContent";
    }

    Serial.println("Konfiguration geladen.");
    Serial.println("Nutze Modell-URL: " + API_URL); // Zur Kontrolle im Monitor
  } else {
    Serial.println("Fehler: config.txt nicht gefunden! STOP.");
    setStatusLED(255, 0, 0); 
    while(1);
  }

  // Lade Prompt aus prompt.txt
  File promptFile = SD_MMC.open("/prompt.txt");
  if(promptFile){ 
    promptFromFile = promptFile.readString(); 
    promptFile.close();
    promptFromFile.trim();
    Serial.println("Prompt geladen: " + promptFromFile);
  } else { 
    promptFromFile = "Beschreibe dieses Bild."; 
  }

  // WLAN verbinden
  Serial.print("Verbinde mit WLAN");
  WiFi.begin(WIFI_SSID.c_str(), WIFI_PASS.c_str());
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); setStatusLED(0, 0, 0); delay(100); setStatusLED(255, 165, 0); }
  Serial.println("\nVerbunden!");
  
  // Peripherie initialisieren
  mySerial.begin(9600, SERIAL_8N1, PRINTER_RX_PIN, PRINTER_TX_PIN); 
  printer.begin();
  printer.setCharset(CHARSET_GERMANY);
  printer.println("Drucker bereit.");
  printer.feed(1);

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0; config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM; config.pin_d1 = Y3_GPIO_NUM; config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM; config.pin_d4 = Y6_GPIO_NUM; config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM; config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM; config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM; config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM; config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM; config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 10000000; 
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_5MP; 
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 1;
  config.fb_count = 1;
  
  if(psramFound()){
    config.jpeg_quality = 10;
    config.fb_count = 2;
    config.grab_mode = CAMERA_GRAB_LATEST;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Kamera-Initialisierung fehlgeschlagen mit Fehler 0x%x\n", err);
    setStatusLED(255, 0, 0);
    return;
  }
  
  // Zusätzliche Sensor-Einstellungen aus dem Beispiel für bessere Bildqualität
  sensor_t * s = esp_camera_sensor_get();
  s->set_vflip(s, 1);
  s->set_brightness(s, 1);
  s->set_saturation(s, 0);

  Serial.println("\nSystem bereit. Bitte Knopf drücken.");
  printer.println("System bereit.");
  printer.feed(2);
  setStatusLED(0, 0, 255); 
}

// === Hauptschleife ===
void loop() {
  if (digitalRead(BUTTON_PIN) == LOW) {
    setStatusLED(255, 255, 0); 
    Serial.println("Knopf gedrückt! Starte Prozess...");
    camera_fb_t * fb = esp_camera_fb_get();
    if (fb) {
      savePhoto(fb);
      uploadAndAnalyze(fb);
      esp_camera_fb_return(fb);
    } else {
      Serial.println("Fehler bei der Bildaufnahme");
    }
    Serial.println("\nProzess abgeschlossen. Warte auf nächsten Knopfdruck.");
    drucke("Bereit für nächstes Bild.");
    printer.feed(2);
    setStatusLED(0, 0, 255);
    delay(1000); 
  }
}

// === Hilfsfunktionen ===
void savePhoto(camera_fb_t * fb) {
  int n = 0;
  String path;
  do {
    path = "/bild_" + String(n++) + ".jpg";
  } while (SD_MMC.exists(path));
  Serial.printf("Nächster freier Dateiname: %s\n", path.c_str());
  File file = SD_MMC.open(path.c_str(), FILE_WRITE);
  if (file) { 
      file.write(fb->buf, fb->len); 
      file.close(); 
      Serial.printf("Bild erfolgreich gespeichert: %s\n", path.c_str()); 
  } else {
      Serial.println("FEHLER: Konnte Datei auf SD-Karte nicht zum Schreiben öffnen!");
  }
}

// API-FUNKTION
void uploadAndAnalyze(camera_fb_t *fb) {
  String fileUri = "";
  HTTPClient http;
  String uploadUrl = "https://generativelanguage.googleapis.com/upload/v1beta/files";
  http.begin(uploadUrl);
  http.setTimeout(30000);
  http.addHeader("X-Goog-Api-Key", API_KEY);
  http.addHeader("Content-Type", "image/jpeg");
  Serial.printf("Starte Upload von %d bytes...\n", fb->len);
  int httpCode = http.sendRequest("POST", fb->buf, fb->len);
  if (httpCode == HTTP_CODE_OK) {
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, http.getStream());
    fileUri = doc["file"]["uri"].as<String>();
    Serial.println("Upload erfolgreich! URI: " + fileUri);
  } else {
    Serial.printf("[HTTP] Upload fehlgeschlagen, Fehler-Code: %d\n", httpCode);
    Serial.println(http.getString()); http.end(); return;
  }
  http.end();
  if (fileUri.isEmpty()) { Serial.println("Konnte keine fileUri extrahieren."); return; }
  String analyzeUrl = API_URL + "?key=" + API_KEY;
  DynamicJsonDocument jsonPayload(1024);
  JsonObject contents = jsonPayload.createNestedObject("contents");
  JsonArray parts = contents.createNestedArray("parts");
  JsonObject part1 = parts.createNestedObject();
  part1["text"] = promptFromFile;
  JsonObject part2 = parts.createNestedObject();
  JsonObject fileData = part2.createNestedObject("fileData");
  fileData["mimeType"] = "image/jpeg";
  fileData["fileUri"] = fileUri;
  String payload;
  serializeJson(jsonPayload, payload);
  http.begin(analyzeUrl);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(30000);
  Serial.println("Sende Analyse-Anfrage...");
  httpCode = http.POST(payload);
  if (httpCode == HTTP_CODE_OK) {
    String responseBody = http.getString();
    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, responseBody);
    if (error) {
      Serial.print("JSON-Parsing fehlgeschlagen: "); Serial.println(error.c_str());
      drucke("Fehler: Antwort unlesbar.");
      setStatusLED(255, 0, 0);
    } else if (doc.containsKey("candidates")) {
      const char* text = doc["candidates"][0]["content"]["parts"][0]["text"];
      Serial.println("\n--- Antwort von Gemini ---");
      Serial.println(text); Serial.println("--------------------------");
      setStatusLED(0, 255, 0);
      printer.boldOn(); printer.println("Analyse von Gemini:"); printer.boldOff(); printer.feed(1);
      drucke(text);
    } else {
      Serial.println("Fehler: 'candidates' wurde in der Antwort nicht gefunden.");
      Serial.println("Komplette Server-Antwort zur Fehlersuche:"); Serial.println(responseBody);
      setStatusLED(255, 0, 0);
      drucke("Fehler: Inhaltl. Antwort.");
    }
  } else {
    Serial.printf("[HTTP] Analyse-Anfrage fehlgeschlagen, Fehler-Code: %d\n", httpCode);
    Serial.println(http.getString());
    setStatusLED(255, 0, 0);
    drucke("Fehler: HTTP-Anfrage.");
  }
  http.end();
}