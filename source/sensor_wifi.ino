#include <ESP8266WiFi.h>
#include <ArduinoJson.h>

#define MYID "A"
#define MSG_BUFSIZ 200

char ssid[] = "ND-guest";
char password[] = "";
WiFiServer server(80);

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

      char info_message[MSG_BUFSIZ];
      sprintf(info_message, "{\"id\":\"%s\", \"type\":\"info\",\"message\":\"Client connected to server\"}", MYID);
      Serial.println(info_message);
      Serial.flush();

      // receives the message from the client
      // But we don't care what it is because we always just return sensor vals
      String request = client.readStringUntil('\r');
      client.flush();

      // Read any current buffered data in and discard
      while(Serial.available()) {
        Serial.readString();
      }

      // Request Info from Uno:
      request_info();

      // Reading the response from Uno:
      String message = "";
      bool messageReady = false;
      while(Serial.available()) {
        message = Serial.readStringUntil('\n');
        messageReady = true;
      }
    
      if (messageReady) {
        Serial.print("{\"id\":\"");
        Serial.print(MYID);
        Serial.print("\", \"type\":\"info\",\"message\":\"Read response from Uno: ");
        Serial.print(message);
        Serial.println("\"}");
        Serial.flush();

        client.print(message);
        client.println("\r"); // sends the answer to the client
      }
    }
    client.stop();
  }
}

void request_info() {

  // Sending the request to the uno via the serial
  DynamicJsonDocument doc(100);
  doc["id"] = MYID;
  doc["type"] = "request";
  char request[MSG_BUFSIZ];
  serializeJson(doc,request);
  sprintf(request, "%s\n", request);
  Serial.write(request);
  Serial.flush();
}
