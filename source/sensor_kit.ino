#include <DHT.h>
#include <Wire.h>
#include "Si115X.h"

#define MOISTPIN 0
#define DRY_BOUND 523 // from calibration - soil moisture sensor output when just exposed to air - our humidity = 0%RH
#define WET_BOUND 254 // from calibration - soil moisture sensor output when in a cup of water - our humidity = 100%RH
#define DHTTYPE DHT22
#define DHTPIN 4
#define RXPIN 2
#define TXPIN 3
#define KEYPIN 9 // key pin of bluetooth

// Initialize objects
DHT dht(DHTPIN, DHTTYPE);
Si115X si1151;

// declare globals
float moist, hum, temp, ir, vis, uv;

void setup() {

  Serial.begin(9600); // start serial for output

  // hum/temp
  dht.begin();

  // sunlight
  Wire.begin();
  if (!si1151.Begin()) {
      Serial.println("Si1151 is not ready!");
  } else {
      Serial.println("Si1151 is ready!");
  }
}

void loop() {
  
  // update readings
  read_moist();
  read_hum_temp();
  read_sunlight();  

  // ERROR CHECKING -- check for -1's

  // output to serial monitor
  output_serial();
  
  delay(1000);
}

// Reads the analog output from the soil moisture sensor, calculates the moisture level as a percentage, and saves it to the variable moist
void read_moist() {
  float moist_val = analogRead(MOISTPIN); //connect moisture sensor to Analog 0
  moist = min(100.0, max(0.0, (1.0 - max(0.0,(moist_val - WET_BOUND))/(DRY_BOUND - WET_BOUND))*100.0)); // define "percentage moist" from upper and lower bounds from calibration
}

// Uses the DHT library to read the humidity and temperature sensor, saves humidity percentage reading to hum and temperature reading (converted to Fahrenheit) to temp
void read_hum_temp() {
  float hum_temp_val[2] = {0}; // (humidity, temperature)
  if (dht.readTempAndHumidity(hum_temp_val)) {
    // spit error vals is func returns 1
    hum_temp_val[0] = -1.0;
    hum_temp_val[1] = -1.0;
  }
  hum_temp_val[1] = (9.0*hum_temp_val[1])/5.0 + 32.0; // convert to fahrenheit

  hum = hum_temp_val[0];
  temp = hum_temp_val[1];
}

// Uses si1151 library to get the infrared, visible, and ultraviolet light readings (in lumens) and saves them to ir, vis, and uv respectively
void read_sunlight() {
  ir = si1151.ReadHalfWord();
  vis = si1151.ReadHalfWord_VISIBLE();
  uv = si1151.ReadHalfWord_UV();
}

// Outputs sensor values to serial monitor
void output_serial() {
  Serial.print("Moisture: ");
  Serial.print(moist);
  Serial.print("%\t Humidity: ");
  Serial.print(hum);
  Serial.print("%\t Temperature: ");
  Serial.print(temp);
  Serial.print(" F\t IR: ");
  Serial.print(ir);
  Serial.print(" lum\t Visible: ");
  Serial.print(vis);
  Serial.print(" lum\t UV: ");
  Serial.print(uv);
  Serial.println(" lum");
}
