#include <string.h>
#include <DHT.h>
#include <Wire.h>
#include "Si115X.h"
#include <ArduinoJson.h>

/* Pinout:
- A0 -> moisture sensor output
- A1 -> pH sensor output
- 4  -> hum/temp sensor output
*/

#define MYID "A"
#define MOISTPIN 0
#define DRY_BOUND 523 // from calibration - soil moisture sensor output when just exposed to air - our humidity = 0%RH
#define WET_BOUND 254 // from calibration - soil moisture sensor output when in a cup of water - our humidity = 100%RH
#define PHPIN A1
#define DHTTYPE DHT22 // hum temp sensor type
#define DHTPIN 4


// Initialize objects
//SoftwareSerial BTSerial(RXPIN, TXPIN);
DHT dht(DHTPIN, DHTTYPE);
Si115X si1151;

// declare globals
int Ntxpct = 0;

float moist, hum, temp, ir, vis, uv, ph;

float avg_moist = 0;
float avg_hum = 0;
float avg_temp = 0;
float avg_ir = 0;
float avg_vis = 0;
float avg_uv = 0;
float avg_ph = 0;

int n_readings = 0; // will incrementally go up every time a reading is taken before it's transmitted

// timing related:
unsigned long check_sens_timer;
const unsigned long check_sens_interval = 1000; // in milliseconds

void setup() {

  Serial.begin(9600); // start serial for output

  // Bluetooth HC-05
  //BTSerial.begin(38400); // why this

  // hum/temp
  dht.begin();

  // sunlight
  Wire.begin();
  if (!si1151.Begin()) {
      Serial.println("Si1151 is not ready!");
  } else {
      Serial.println("Si1151 is ready!");
  }

  // set timer
  check_sens_timer = set_timer();
}

void loop() {
  
  // RESPOND TO REQUEST FROM SHIELD (from controller)
  if (Serial.available()) {
    int err_code = read_request();
    if (err_code == 0 || err_code == 1) {
      send_response(err_code);

      zero_out_avgs();
      n_readings = 0;
    }
  }

  // SENSORS
  // check if it's been 1 check_sens_interval since the last time we accessed the sensors
  if (check_timer(check_sens_timer, check_sens_interval)) {
    check_sens_timer = set_timer();

    // get readings
    read_moist();
    read_hum_temp();
    read_sunlight();
    read_ph();

    // ERROR CHECKING -- check for -1's
    // could also make temp sensor check for temp operational bounds for other sensors

    // update averages:
    n_readings++;
    avg_moist = update_avg(moist, avg_moist, n_readings);
    avg_hum = update_avg(hum, avg_hum, n_readings);
    avg_temp = update_avg(temp, avg_temp, n_readings);
    avg_ir = update_avg(ir, avg_ir, n_readings);
    avg_vis = update_avg(vis, avg_vis, n_readings);
    avg_uv = update_avg(uv, avg_uv, n_readings);
    avg_ph = update_avg(ph, avg_ph, n_readings);
  }
}

/*    ###############################   TIMING     #########################   */

unsigned long set_timer() {
  unsigned long timer = millis();
  return timer;
}

bool check_timer(unsigned long timer, unsigned long interval) {
  if ((millis() - timer) >= interval) {
    return 1;
  }
  return 0;
}

/*    ###############################   BLUETOOTH  #########################   */

void increment_Ntranspacket() {
  Ntxpct++;

  // Keep under 100 so we only take up 2 chars in packet
  if (Ntxpct >= 100) {
    Ntxpct = 0;
  }
}

int read_request() {

  bool messageReady = false;
  String message = "";

  if (Serial.available()) {
    message = Serial.readString();
    messageReady = true;
  }

  if (messageReady) {
    StaticJsonDocument<100> doc;
    DeserializationError error = deserializeJson(doc, message);
    if(error) {
      return 1;
    }
    if(doc["type"] != "request") {
      return 2;
    }
  } else {
    return -1;
  }

  return 0;
}

void send_response(int err_code) {

  StaticJsonDocument<100> doc;

  doc["id"] = MYID;
  doc["packet number"] = Ntxpct;
  doc["type"] = "response";

  if (err_code == 1) {
    doc["message"] = "Deserialize Json Failed";
  } else {
    doc["moisture"] = avg_moist;
    doc["humidity"] = avg_hum;
    doc["temperature"] = avg_temp;
    doc["ir"] = avg_ir;
    doc["vis"] = avg_vis;
    doc["uv"] = avg_uv;
    doc["ph"] = avg_ph;
  }

  serializeJson(doc, Serial);

  increment_Ntranspacket();
}

/*    ###############################   SENSOR READINGS AND OUTPUT  #########################   */

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

// Converts voltage reading to pH - 2.5 is neutral voltage reading, so multiply 2.5-voltage by calibrated slope 1/0.18 and add offset of 7
float volt2ph (float voltage) {
    return 7 + ((2.5 - voltage) / 0.18);
}

// Reads pH from sensor and saves to ph
void read_ph() {
  float voltage = 5.0 / 1024.0 * analogRead(PHPIN); // 5 for 5V, 1024 adc resolution
  ph = volt2ph(voltage);
}

void zero_out_avgs() {
  avg_moist = 0.0;
  avg_hum = 0.0;
  avg_temp = 0.0;
  avg_ir = 0.0;
  avg_vis = 0.0;
  avg_uv = 0.0;
  avg_ph = 0.0;
}

float update_avg(float new_value, float avg_value, int n_readings) {
  return (avg_value*(float)(n_readings - 1) + new_value)/(float)n_readings;
}
