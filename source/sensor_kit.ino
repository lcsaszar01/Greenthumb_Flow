#include <DHT.h>
#include <LiquidCrystal.h>

#define DHTTYPE DHT22
#define DHTPIN 4

DHT dht(DHTPIN, DHTTYPE);

int RSpin = 5; //give pin names for LCD
int ENpin = 7;
int D4pin = 8;
int D5pin = 10;
int D6pin = 11;
int D7pin = 12;

LiquidCrystal lcd(RSpin,ENpin,D4pin,D5pin,D6pin,D7pin);

float dry_bound = 523.0; // output when just exposed to air - our humidity = 0%RH
float wet_bound = 254.0; // output when in a cup of water - our humidity = 100%RH

void setup() {
  Wire.begin();
  dht.begin();
  lcd.begin(16,2); //start LCD and create the defined characters
  lcd.print("moisture: ");
  lcd.setCursor(0,1);
  lcd.print("temp: ");
}

void loop() {

  float temp_hum_val[2] = {0};
  float moist_val;

  moist_val = analogRead(0); //connect moisture sensor to Analog 0

  if (dht.readTempAndHumidity(temp_hum_val)) {
    temp_hum_val[0] = -1.0;
    temp_hum_val[1] = -1.0;
  }

  float moist_perc = min(100.0, max(0.0, (1.0 - max(0.0,(moist_val - wet_bound))/(dry_bound - wet_bound))*100.0));
  
  lcd.setCursor(10,0);
  lcd.print(moist_perc);

  lcd.setCursor(6,1);
  lcd.print((9.0*temp_hum_val[1])/5.0 + 32.0);
  //lcd.print(temp_hum_val[0]);

  delay(100);
}
