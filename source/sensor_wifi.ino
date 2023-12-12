#include <ESP8266WiFi.h>
#include <ArduinoJson.h>

char ssid[] = "ND-guest";
char password[] = "";
WiFiServer server(80);

String message = "";

void setup() {
  WiFi.begin(ssid,password);
  Serial.begin(9600);
  while(WiFi.status()!=WL_CONNECTED)
  {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  server.begin();

}

void loop () {
  WiFiClient client = server.available();
  if (client) {
    if (client.connected()) {

      // Request Info from Uno:
      request_info();

      String request = client.readStringUntil('\r');    // receives the message from the client
      client.flush();
      client.print(message);
      client.println("\r"); // sends the answer to the client
    }
    client.stop();
  }
}

void request_info() {

  DynamicJsonDocument doc(100);

  // Sending the request
  doc["type"] = "request";
  serializeJson(doc,Serial);
  // Reading the response
  boolean messageReady = false;
  while(messageReady == false) { // blocking but that's ok
    if(Serial.available()) {
      message = Serial.readString();
      messageReady = true;
    }
  }
}
