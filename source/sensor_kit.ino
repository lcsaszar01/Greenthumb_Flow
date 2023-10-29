#include <DHT.h>
//#include <LiquidCrystal.h>
#include <Wire.h>
#include "Si115X.h"

#define DHTTYPE DHT22
#define DHTPIN 4

DHT dht(DHTPIN, DHTTYPE);

Si115X si1151;

//int RSpin = 5; //give pin names for LCD
//int ENpin = 7;
//int D4pin = 8;
//int D5pin = 10;
//int D6pin = 11;
//int D7pin = 12;

//LiquidCrystal lcd(RSpin,ENpin,D4pin,D5pin,D6pin,D7pin);

float dry_bound = 523.0; // output when just exposed to air - our humidity = 0%RH
float wet_bound = 254.0; // output when in a cup of water - our humidity = 100%RH

byte x = 0;

void setup() {

  Serial.begin(9600);           // start serial for output

  dht.begin();

  uint8_t conf[4];

  Wire.begin();
  if (!si1151.Begin())
      Serial.println("Si1151 is not ready!");
  else
      Serial.println("Si1151 is ready!");

  //lcd.begin(16,2); //start LCD and create the defined characters
  //lcd.print("moisture: ");
  //lcd.setCursor(0,1);
  //lcd.print("temp: ");
}

void loop() {
  
  // SOIL MOISTURE
  float moist_val;
  moist_val = analogRead(0); //connect moisture sensor to Analog 0
  float moist_perc = min(100.0, max(0.0, (1.0 - max(0.0,(moist_val - wet_bound))/(dry_bound - wet_bound))*100.0));

  // TEMPERATURE / HUMIDITY
  float hum_temp_val[2] = {0}; // (humidity, temperature)
  if (dht.readTempAndHumidity(hum_temp_val)) {
    // spit error vals is func returns 1
    hum_temp_val[0] = -1.0;
    hum_temp_val[1] = -1.0;
  }
  hum_temp_val[1] = (9.0*hum_temp_val[1])/5.0 + 32.0; // convert to fahrenheit

  // SUNLIGHT
  float ir = si1151.ReadHalfWord();
  float vis = si1151.ReadHalfWord_VISIBLE();
  float uv = si1151.ReadHalfWord_UV();

  // OUTPUT
  Serial.print("Moisture: ");
  Serial.print(moist_perc);
  Serial.print("%\t Humidity: ");
  Serial.print(hum_temp_val[0]);
  Serial.print("%\t Temperature: ");
  Serial.print(hum_temp_val[1]);
  Serial.print(" F\t IR: ");
  Serial.print(ir);
  Serial.print(" lum\t Visible: ");
  Serial.print(vis);
  Serial.print(" lum\t UV: ");
  Serial.print(uv);
  Serial.println(" lum");
  
  //lcd.setCursor(10,0);
  //lcd.print(moist_perc);

  //lcd.setCursor(6,1);
  //lcd.print((9.0*temp_hum_val[1])/5.0 + 32.0);
  //lcd.print(temp_hum_val[0]);

  delay(1000);
}
