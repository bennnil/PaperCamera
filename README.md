# PaperCamera AI 📷✨

Eine Sofortbildkamera basierend auf dem ESP32-S3, die Fotos nicht ausdruckt, sondern sie vorher von einer KI (Google Gemini) analysieren lässt.
Das Projekt ist im Rahmen des Unterrichts entstanden und wurde hier dokumentiert: http://www.tinkeredu.de/informatik/ki-und-klimaschutz-widerspruch-oder-werkzeug/

![Status](https://img.shields.io/badge/Status-Stable-green)

## Funktionen
* **Snapshot & AI:** Macht ein Foto und sendet es an Google Gemini.
* **Thermal Print:** Druckt die Beschreibung, einen Witz oder ein Gedicht über das Bild aus.
* **Smart Feedback:** Eine farbige LED zeigt genau an, was die Kamera gerade macht (oder welcher Fehler vorliegt).
* **SD-Card Config:** WLAN und API-Keys werden sicher auf der SD-Karte gespeichert, nicht im Code.

## Hardware-Liste
1. **ESP32-S3 Board:** z.B. https://de.aliexpress.com/item/1005008589091526.html?spm=a2g0o.order_list.order_list_main.63.5aa75c5fESzsB8&gatewayAdapt=glo2deu 
2. **Kamera:** OV2640 im Kit enthalten (es bietet sich an, eine Kamera mit langem Kabel zu nutzen, um die Verkabelung zu vereinfachen)
3. **Thermodrucker:** TTL Serial Printer z.B. https://de.aliexpress.com/item/1005007402905573.html?spm=a2g0o.order_list.order_list_main.125.5aa75c5fESzsB8&gatewayAdapt=glo2deu 
4. **MicroSD Karte:** Max 16GB empfohlen (FAT32) Ich habe eine Verlängerung genutzt z.B. https://de.aliexpress.com/item/4001200431510.html?spm=a2g0o.order_list.order_list_main.83.5aa75c5fESzsB8&gatewayAdapt=glo2deu
5. **Button & NeoPixel:** Ein Taster und eine WS2812B LED z.B. https://www.alibaba.com/product-detail/16mm-Stainless-Steel-Waterproof-Wired-Ws2812_1601446043870.html?spm=a2756.order-detail-ta-bn-b.0.0.3e61f19cDmsLW4

Ich habe ein PCB erstellt, um den Button und den Drucker leichter verbinden zu können.


## Installation

### 1. Gemini API Key besorgen
https://ai.google.dev/gemini-api/docs/api-key?hl=de

### 2. Hotspot einrichten
Die Kamera benötigt eine Internetverbindung. Ein Hotspot auf dem Smartphone reicht vollkommen aus. 

### 3. SD-Karte vorbereiten
Erstelle folgende Dateien im Hauptverzeichnis der SD-Karte:

**`config.txt`** (4 Zeilen):
```text
DEIN_WLAN_NAME
DEIN_WLAN_PASSWORT
DEIN_GEMINI_API_KEY
https://generativelanguage.googleapis.com/v1beta/models/gemini-2.5-flash:generateContent
```

**`prompt.txt`** (1 Zeile):
```text
Beschreibe was du siehst sehr kurz und sarkastisch.
```

### 4. Geh raus in die Welt und mache Fotos. Die Fotos werden auch auf der SD Karte gespeichert.

## Troubeshooting
Der Drucker benötigt viel Strom, sodass die Versorgung über eine Powerbank (abhängig von der Powerbank) nicht funktioniert. Ich habe eine 18650 Lipo mit einem entsprechenden Ladeboard das 2A bereitstellen kann genutzt.
