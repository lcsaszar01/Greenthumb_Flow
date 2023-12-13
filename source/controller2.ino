#include <ArduinoJson.h>

#define MYID "CTLR UNO"
#define SOL_A_PIN 7
#define SOL_B_PIN 8

String err_message = "";
String message = "";

void setup()
{
  pinMode(SOL_A_PIN, OUTPUT);
  pinMode(SOL_B_PIN, OUTPUT);
  Serial.begin(9600);
}

void loop()
{

  bool ignore = false;
  err_message = "OK";
  //bool messageReady = false;

  // RESPOND TO REQUEST FROM SHIELD (from server)
  while (!Serial.available()) {}

  message = Serial.readStringUntil('\n');

  //if (messageReady){
  Serial.print("received message: ");
  Serial.println(message);
  Serial.flush();
  StaticJsonDocument<100> doc;
  DeserializationError error = deserializeJson(doc, message);
  if(error) {
    // Print response so shield can see
    err_message = "DeserializeJson failed";
  } else {
    if (!doc.containsKey("type")) {
      err_message = "Type field missing";
    } else {

      if (doc["type"] == "info") {
        ignore = true;
      } else {
        if (!doc.containsKey("zone") || !doc.containsKey("amount (L)")) {
          err_message = "Missing ZONE or AMOUNT (L) keys";
        }

        // If no error, call solenoid ctl based on message
        int error = solenoid_ctl(doc["zone"], doc["amount (L)"]);
        if (error) {
          err_message = "Solenoid control unsuccessful";
        }
      }
    }
    //}

    if (!ignore) {
      // Ack in serial
      StaticJsonDocument<100> resp_doc;
      resp_doc["id"] = MYID;
      resp_doc["type"] = "response";
      resp_doc["message"] = err_message;
      serializeJson(resp_doc, Serial);
      Serial.println("");
      Serial.flush();
    }
  }
}

int solenoid_ctl(String zone, float amount) {

// L to mL multiplied by ratio (7.5 mL per second)
// now in seconds, convert to ms for delay function

int ms = round(((amount/1000)*7.5)/1000);

if (!strcmp((zone.c_str()), "A")) {
    digitalWrite(SOL_A_PIN, HIGH);
    delay(ms);
    digitalWrite(SOL_A_PIN, LOW);
} 
else {
    digitalWrite(SOL_B_PIN, HIGH);
    delay(ms);
    digitalWrite(SOL_B_PIN, LOW);
}

  return 0;
}
