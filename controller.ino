    #include <SoftwareSerial.h>
    #define rxPin 10
    #define txPin 11
    byte incoming;
    SoftwareSerial nodeCommunication = SoftwareSerial(rxPin, txPin);

    void setup()
    {
    Serial.begin(9600);
    pinMode(rxPin, INPUT);
    pinMode(txPin, OUTPUT);
    nodeCommunication.begin(9600); 
    }

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