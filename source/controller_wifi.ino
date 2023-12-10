#include <SPI.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>

#define MYID "CTLR"

char ssid[] = "ND-guest";
char password[] = "";

IPAddress sensorA_server(10,7,54,33);       // the fix IP address of the server
WiFiClient sensorA_client;
WiFiServer controller_server(80);

int Ntxpct_sensor = 0;
int Ntxpct_server = 0;

String sens_answer = "";

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
  controller_server.begin();

}

void loop () {
  WiFiClient controller_client = controller_server.available();
  if (controller_client) {
    if (controller_client.connected()) {

      String request = controller_client.readStringUntil('\r');    // receives the message from the client
      controller_client.flush();
      StaticJsonDocument<100> serv_doc;
      serv_doc["id"] = "SERV";
      serv_doc["packet number"] = 0;
      serv_doc["type"] = "request";
      serv_doc["request_type"] = "sensor";
      //serv_doc["zone"] = "A";
      //serv_doc["amount (L)"] = 450;
      request = "";
      serializeJson(serv_doc, request);
      int err_code = read_request(request);
      if (err_code >= 0) {
        String response = "";

        StaticJsonDocument<100> doc;

        doc["id"] = MYID;
        doc["packet number"] = Ntxpct_server;
        doc["type"] = "response";
        doc["message"] = message;

        serializeJson(doc, response);
        controller_client.print(response + "\r"); // sends the answer to the client
      }
    }
    controller_client.stop();
  }

  // Also read Serial and check for imp. messages

}

void increment_Ntranspacket_server() {
  Ntxpct_server++;

  // Keep under 100 so we only take up 2 chars in packet
  if (Ntxpct_server >= 100) {
    Ntxpct_server = 0;
  }
}

void increment_Ntranspacket_sensor() {
  Ntxpct_sensor++;

  // Keep under 100 so we only take up 2 chars in packet
  if (Ntxpct_sensor >= 100) {
    Ntxpct_sensor = 0;
  }
}

int read_request(String request) {

  StaticJsonDocument<300> doc;
  DeserializationError error = deserializeJson(doc, request);
  if(error) {
    message = "DeserializeJson Failed";
    return 1;
  }

  if (!doc.containsKey("type") || !doc.containsKey("request_type")) {
    message = "Invalid format";
    return 2;
  }

  if(doc["type"] != "request") {
    return -1;
  }

  // could check packet number

  if (doc["request_type"] == "sensor") {
    request_sensor_info();

  } else if (doc["request_type"] == "solenoid") {

    if (!doc.containsKey("zone") || !doc.containsKey("amount (L)")) {
      message = "Missing necessary keys for solenoid control";
      return 3;
    }
    send_solenoid_ctl_msg(doc["zone"], doc["amount"]);

  } else {
    message = "Invalid request type";
    return 4;
  }

  message = "OK";
  return 0;
}

// Send solenoid control message to Uno via shared Serial
void send_solenoid_ctl_msg(String zone, float amount) {
  StaticJsonDocument<100> doc;

  doc["id"] = MYID;
  doc["type"] = "request";
  doc["zone"] = zone;
  doc["amount (L)"] = amount;

  serializeJson(doc, Serial);
}

void request_sensor_info() {

  // Theoretically loop to all zone

  String sensor_request = "";

  StaticJsonDocument<100> doc;

  doc["id"] = MYID;
  doc["packet number"] = Ntxpct_sensor;
  doc["type"] = "request";
  doc["zone"] = "A";

  serializeJson(doc, sensor_request); // you also don't need to do this bc it'll send you output no matter how you format your message as it is rn

  increment_Ntranspacket_sensor();
  sensorA_client.connect(sensorA_server, 80);   // Connection to the server
  sensorA_client.print(sensor_request);
  sensorA_client.println("\r");  // sends the message to the server
  sens_answer = sensorA_client.readStringUntil('\r');   // receives the answer from the sever
  Serial.print("Sensor answer: ");
  Serial.println(sens_answer);
  sensorA_client.flush();
}
