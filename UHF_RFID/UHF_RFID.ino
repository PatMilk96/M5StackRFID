
#include "RFID_command.h"
#include <M5Atom.h>
UHF_RFID RFID;

String comd = " ";
CardpropertiesInfo card;
ManyInfo cards;
SelectInfo Select;
CardInformationInfo Cardinformation;
QueryInfo Query;
ReadInfo Read;
TestInfo Test;


#include <WiFi.h>
#include <HTTPClient.h>
#include<ArduinoJson.h>

const char* ssid = "PatsPhone";
const char* password = "Zqp5ED!9";
char jsonOutput[128];
int x = 0;

void setup() {
  M5.begin(true, false, true);

  RFID._debug = 0;
  Serial2.begin(115200, SERIAL_8N1, 32, 26);
  if (RFID._debug == 1) Serial.begin(115200, SERIAL_8N1, 21, 22);

  RFID.Set_transmission_Power(2600);
  RFID.Set_the_Select_mode();
  RFID.Delay(100);
  RFID.Readcallback();
  RFID.clean_data();

  // Prompted to connect to UHF_RFID
  Serial.println("Please connect UHF_RFID to Port C");

  // Determined whether to connect to UHF_RFID
  String soft_version;
  soft_version = RFID.Query_software_version();
  while (soft_version.indexOf("V2.3.5") == -1) {
    RFID.clean_data();
    RFID.Delay(150);
    RFID.Delay(150);
    soft_version = RFID.Query_software_version();
  }

  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    Serial.println(".");
    delay(500);
  }
  Serial.println("\nConnected to WiFi");
  Serial.println("\nIP address: ");
  Serial.println(WiFi.localIP());
}


void loop() {
  const int MAX_CARDS = 10;
  const unsigned long SCAN_TIMEOUT = 2000;

  ManyInfo cards = RFID.Multiple_polling_instructions(MAX_CARDS);

  size_t numCards = cards.len;

  if (cards.len > 0) {
    M5.dis.fillpix(0xff0000);
  } else {
    M5.dis.fillpix(0x00ff00);
  }

  String otherTags[MAX_CARDS];
  int otherTagsCount = 0;

  String userTag;

  for (size_t i = 0; i < numCards; i++) {
    if (cards.card[i]._EPC.length() == 24) {
      // Print RFID card information
      Serial.println("RSSI: " + cards.card[i]._RSSI);
      Serial.println("PC: " + cards.card[i]._PC);
      Serial.println("EPC: " + cards.card[i]._EPC);
      Serial.println("CRC: " + cards.card[i]._CRC);

      // Check if it's the userTag
      if (cards.card[i]._EPC == "e280699500004009f12729a1") {
        userTag = cards.card[i]._EPC;
      } else {
        otherTags[otherTagsCount] = cards.card[i]._EPC;
        otherTagsCount++;
      }
    }
  }

  delay(SCAN_TIMEOUT);


  if (otherTagsCount > 0) {
    // If no new tags were found, make the POST
    const size_t DOC_CAPACITY = JSON_OBJECT_SIZE(2) + JSON_ARRAY_SIZE(MAX_CARDS) + JSON_OBJECT_SIZE(1);
    StaticJsonDocument<DOC_CAPACITY> doc;

    JsonObject tag = doc.createNestedObject("tag");
    tag["tag"] = userTag;

    JsonArray otherTagsArray = doc.createNestedArray("otherTags");
    for (int i = 0; i < otherTagsCount; i++) {
      otherTagsArray.add(otherTags[i]);
    }

    String jsonOutput;
    serializeJson(doc, jsonOutput);

    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient client;
      client.begin("http://34.205.33.41:3010/buyProduct");
      client.addHeader("Content-Type", "application/json");
      int httpCode = client.POST(jsonOutput);
      if (httpCode > 0) {
        String payload = client.getString();
        Serial.println("\nStatuscode: " + String(httpCode));
        Serial.println(payload);
        client.end();
      } else {
        Serial.println("Error on HTTP request");
        Serial.println(httpCode);
      }
    } else {
      Serial.println("Connection lost");
    }
  } else {
    Serial.println("No tags other than the userTag were scanned. Skipping POST request.");
  }

  RFID.clean_data();
  numCards = 0;
}
