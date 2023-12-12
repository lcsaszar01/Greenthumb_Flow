#include <SPI.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>

#define MYID "CTLR"
#define MSGBUF 200

char ssid[] = "ND-guest";
char password[] = "";

IPAddress sensorA_server(10,7,54,33);       // the fix IP address of the server
WiFiClient sensorA_client;
WiFiServer controller_server(80);

int Ntxpct_sensor = 0;
int Ntxpct_server = 0;

char sens_answer[MSGBUF];

//String message = "";
char message[MSGBUF];

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

      char info_message[MSGBUF];
      sprintf(info_message, "{\"id\":\"%s\", \"type\":\"info\",\"message\":\"Client connected to controller server\"}", MYID);
      Serial.println(info_message);
      Serial.flush();

      String request = controller_client.readStringUntil('\r');    // receives the message from the client
      controller_client.flush();

      sprintf(info_message, "{\"id\":\"%s\", \"type\":\"info\",\"message\":\"ACTUAL request received: ", MYID);
      Serial.print(info_message);
      Serial.print(request);
      Serial.println("\"}");
      Serial.flush();

      // Constructing message from server until actually set up server connection
      StaticJsonDocument<100> serv_doc;
      serv_doc["id"] = "SERV";
      serv_doc["packet number"] = 0;
      serv_doc["type"] = "request";
      //serv_doc["request_type"] = "sensor";
      serv_doc["request_type"] = "solenoid";
      serv_doc["zone"] = "A";
      serv_doc["amount (L)"] = 450;
      char request2[MSGBUF];
      serializeJson(serv_doc, request2);

      sprintf(info_message, "{\"id\":\"%s\", \"type\":\"info\",\"message\":\"Request received: %s\"}", MYID, request2);
      Serial.println(info_message);
      Serial.flush();

      // Read request
      // read_request modifies global variable 'message' inside the func.
      int err_code = read_request(request2);

      if (err_code >= 0) {
        String response = "";

        StaticJsonDocument<100> doc;

        doc["id"] = MYID;
        doc["packet number"] = Ntxpct_server;
        doc["type"] = "response";
        doc["message"] = message;

        // send info to serial
        sprintf(info_message, "{\"id\":\"%s\", \"type\":\"info\",\"message\":\"response to send to client: %s\"}", MYID, message);
        Serial.println(info_message);
        Serial.flush();

        serializeJson(doc, response);
        controller_client.print(response + "\r"); // sends the answer to the client
      }
    }
    controller_client.stop();
  }
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
    sprintf(message, "DeserializeJson Failed");
    return 1;
  }

  if (!doc.containsKey("type") || !doc.containsKey("request_type")) {
    sprintf(message, "Invalid format");
    return 2;
  }

  if(doc["type"] != "request") {
    char info_message[MSGBUF];
    sprintf(info_message, "{\"id\":\"%s\", \"type\":\"info\",\"message\":\"type field was not request\"}", MYID);
    Serial.println(info_message);
    return -1;
  }

  // could check packet number

  if (doc["request_type"] == "sensor") {

    sprintf(message, "OK - requested sensor info");
    int err_code = request_sensor_info();
    if (err_code) {
      sprintf(message, "Requested sensor info, but failed");
    }

  } else if (doc["request_type"] == "solenoid") {

    if (!doc.containsKey("zone") || !doc.containsKey("amount (L)")) {
      sprintf(message, "Missing necessary keys for solenoid control");
      return 3;
    }

    sprintf(message, "OK - sending solenoid control message");
    send_solenoid_ctl_msg(doc["zone"], doc["amount (L)"]);

  } else {
    sprintf(message, "Invalid request type");
    return 4;
  }

  return 0;
}

// Send solenoid control message to Uno via shared Serial
void send_solenoid_ctl_msg(String zone, float amount) {
  StaticJsonDocument<100> doc;

  doc["id"] = MYID;
  doc["type"] = "request";
  doc["zone"] = zone;
  doc["amount (L)"] = amount;

  char solenoid_ctl_msg[100];

  serializeJson(doc, solenoid_ctl_msg);
  sprintf(solenoid_ctl_msg, "%s\n", solenoid_ctl_msg);
  Serial.write(solenoid_ctl_msg);
  Serial.flush();
}

int request_sensor_info() {

  // Theoretically loop to all zone

  String sensor_request = "";

  char info_message[MSGBUF];

  StaticJsonDocument<100> doc;

  doc["id"] = MYID;
  doc["packet number"] = Ntxpct_sensor;
  doc["type"] = "request";
  doc["zone"] = "A";

  serializeJson(doc, sensor_request); // you also don't need to do this bc it'll send you output no matter how you format your message as it is rn

  increment_Ntranspacket_sensor();
  sensorA_client.connect(sensorA_server, 80);   // Connection to the server

  if (!sensorA_client.connected()) {
    sprintf(info_message, "{\"id\":\"%s\", \"type\":\"info\",\"message\":\"sensor A client not connected\"}", MYID);
    Serial.println(info_message);
    return -1;
  }

  sensorA_client.print(sensor_request);
  sensorA_client.println("\r");  // sends the message to the server

  sprintf(info_message, "{\"id\":\"%s\", \"type\":\"info\",\"message\":\"Message sent to sensor A: ", MYID);
  Serial.print(info_message);
  Serial.print(sensor_request);
  Serial.println("\"}");

  String sens_str = sensorA_client.readStringUntil('\r');
  sprintf(sens_answer, "%s", sens_str);   // receives the answer from the sever
  sensorA_client.flush();

  sprintf(info_message, "{\"id\":\"%s\", \"type\":\"info\",\"message\":\"Sensor A response: ", MYID);
  Serial.print(info_message);
  Serial.print(sens_str);
  Serial.println("\"}");

  return 0;
}
