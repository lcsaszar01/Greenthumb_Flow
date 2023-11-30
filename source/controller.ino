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
// {2 char transmission ID, 0-99. See added Ntxpct code}{1 char 0 or 1, for whether we want the solenoid off (closed) or on (flow)}{4 character duration of watering if prev field is 1}{null character}
// we will need to adjust the third field depending on how large of a number we figure out we need because obviously right now it's max 9.999 seconds

// Relays sensor data to server to be logged
// taking input from 2 or more sensors
// Adds ID of which sensor it's coming from, and timing info -- parse with DeserializeJson

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
