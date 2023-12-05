#include <SoftwareSerial.h>
#define rxPin 10
#define txPin 11
byte incoming;
SoftwareSerial nodeCommunication = SoftwareSerial(rxPin, txPin);

int Ntxpct_server = 0; // Current number of packets transmitted to server; counts up, resets at 99; so server can verify if any have been dropped
int Ntxpct_sensor = 0; // ditto

void setup()
{
  Serial.begin(9600);
  pinMode(rxPin, INPUT);
  pinMode(txPin, OUTPUT);
  nodeCommunication.begin(9600); 
}

// Transmits solenoid control messages on a schedule determined by server -- solicits updated schedule every 24 hrs would work
// I had an idea to break up the day into 6 4-hr time blocks, where at the start of each time block you either water x amount or don't. Or, water at a certain pace over the time block. First way probably works fine?
// Or 12 2-hr blocks
// Solenoid control messages are of the form:
//"{\"packet number\":0,\"solenoid state\":1, \"solenoid on duration\":2000}"

// Also transmit to the server when you're watering and how much, in HTTP -- will help log it. But may not be necessary, can fill in the log with the schedule for the day that the server will have anyway, and assume the controller carries it out correctly

// Relays sensor data to server to be logged
// taking input from 2 or more sensors
// Adds timing info, removes packet number -- parse with DeserializeJson
// Receive serial data, transmit HTTP
// ChatGPT says:
/*
To make a POST request from an Arduino, you can use the WiFiClient library to establish a connection to the server and the HTTPClient library to handle the HTTP communication. Below is a basic example using an ESP8266 (NodeMCU) board as an example. If you're using a different board, the principles are similar, but you might need to adapt the code.

Firstly, make sure you have the required libraries installed. You can install them through the Arduino Library Manager.

cpp
Copy code
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

const char *ssid = "your-ssid";
const char *password = "your-password";
const char *serverUrl = "http://your-server-url"; // Change this to your server's URL

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Serial.println("Connected to WiFi");
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    // Your server endpoint for the POST request
    String endpoint = "/your-post-endpoint";
    
    // Add any parameters to the POST request body
    String postData = "key1=value1&key2=value2";

    http.begin(serverUrl + endpoint);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    int httpResponseCode = http.POST(postData);

    if (httpResponseCode > 0) {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      String response = http.getString();
      Serial.println(response);
    } else {
      Serial.print("Error in HTTP request: ");
      Serial.println(httpResponseCode);
    }

    http.end();
  }

  delay(5000); // Wait for 5 seconds before making the next request
}
Make sure to replace "your-ssid", "your-password", and "http://your-server-url" with your actual Wi-Fi credentials and server details.

This example sends a simple POST request with key-value pairs as the data. Adjust the postData string to match your specific payload format. Additionally, make sure your server is set up to handle the incoming POST requests at the specified endpoint.
*/

void loop()
{
  if(nodeCommunication.available() > 0) //Only if data is available
  {
    byte incoming = nodeCommunication.read(); //read byte 
    Serial.println("Incoming = ");
    Serial.println(incoming);
  }
  else
  {
    Serial.println("No data available....");
  }

}

void increment_Ntranspacket() {
  Ntxpct++;

  // Keep under 100 so we only take up 2 chars in packet
  if (Ntxpct >= 100) {
    Ntxpct = 0;
  }
}
