#include <ArduinoJson.h>

#define MYID "CTLR UNO"

String err_message = "";
String message = "";

void setup()
{
  Serial.begin(9600);
}

void loop()
{

  // RESPOND TO REQUEST FROM SHIELD (from server)
  if (Serial.available()) {

    err_message = "OK";

    message = Serial.readString();

    StaticJsonDocument<100> doc;
    DeserializationError error = deserializeJson(doc, message);
    if(error) {
      // Print response so shield can see
      err_message = "DeserializeJson failed";
    } else {

      if (!doc.containsKey("zone") || !doc.containsKey("amount (L)"))

      // If no error, call solenoid ctl based on message
      int error = solenoid_ctl(doc["zone"], doc["amount (L)"]);
      if (error) {
        err_message = "Missing ZONE or AMOUNT (L) keys";
      }
    }

    // Ack in serial
    StaticJsonDocument<100> resp_doc;
    resp_doc["id"] = MYID;
    resp_doc["type"] = "response";
    resp_doc["message"] = err_message;
    serializeJson(resp_doc, Serial);
  }
}

int solenoid_ctl(String zone, float amount) {

  // stuff

  return 0;
}
