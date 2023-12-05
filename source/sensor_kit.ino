#include <string.h>
#include <DHT.h>
#include <Wire.h>
#include "Si115X.h"
#include <ArduinoJson.h>

/* Pinout:
- A0 -> moisture sensor output
- A1 -> pH sensor output
- 4  -> hum/temp sensor output
- 7  -> input to transistor that controls solenoid
- 8  -> bluetooth receive pin
- 9  -> bluetooth transmit pin
*/

#define MYID "A"
#define RXPIN 8
#define TXPIN 9
#define ERR_BUFSIZ 100
#define PACKET_BUFSIZ 200
#define STOP_BYTE '\0'
#define MOISTPIN 0
#define DRY_BOUND 523 // from calibration - soil moisture sensor output when just exposed to air - our humidity = 0%RH
#define WET_BOUND 254 // from calibration - soil moisture sensor output when in a cup of water - our humidity = 100%RH
#define PHPIN A1
#define DHTTYPE DHT22 // hum temp sensor type
#define DHTPIN 4
#define SOLENOIDPIN 7
#define FLOW_STATE 1
#define CUTOFF_STATE 0


// Initialize objects
//SoftwareSerial BTSerial(RXPIN, TXPIN);
DHT dht(DHTPIN, DHTTYPE);
Si115X si1151;

// declare globals
int Ntxpct = 0;
int curr_rxpct;
int prev_rxpct = 99;

char err_msg[ERR_BUFSIZ] = "Error.";

char last_trans_ID_recieved;
char last_trans_ID_transmitted;

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
unsigned long transmit_sens_timer;
const unsigned long transmit_sens_interval = 5000; // will change to 1 min later, 5s for testing purposes
unsigned long solenoid_ctl_timer;
const unsigned long solenoid_ctl_interval = 100; // in milliseconds - we leave the water on for time in solenoid_control_interval increments

// solenoid control:
unsigned long solenoid_state_timer;
unsigned long solenoid_state_duration; // will be set upon receiving transmission from controller
bool solenoid_state;
bool solenoid_ctl_received;

void setup() {

  Serial.begin(9600); // start serial for output

  pinMode(SOLENOIDPIN, OUTPUT);

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

  // set defaults
  solenoid_state = CUTOFF_STATE;
  solenoid_ctl_received = 0;
  solenoid_state_duration = 1000;

  // set timers
  check_sens_timer = set_timer();
  transmit_sens_timer = set_timer();
  solenoid_ctl_timer = set_timer();
}

void loop() {
  
  // check for bluetooth transmissions received
  // is it a problem if they arrive during delay()? -- no, there's a buffer. look into length of buffer
  // -- we think the buffer is 63 bytes, and FIFO
  // -- data is lost with a delay longer than 800 ms
  // if solenoid control message received, turn solenoid_ctl_received on, change vals of solenoid_state, and solenoid_state_duration if applic., set solenoid_state_ticker to 0, verify that solenoid_state_duration % solenoid_ctl_interval = 0 or round to make it so

  if (Serial.available()) {
    int err_code = read_packet();
    transmit_ack();
  }

  // SOLENOID CONTROL
  // check if it's been 1 solenoid_ctl_interval since the last time we accessed solenoid control
  if (check_timer(solenoid_ctl_timer, solenoid_ctl_interval)) {
    solenoid_ctl_timer = set_timer();

    // Access solenoid control

    // If the state has changed via bluetooth instruction instead of timeout (done directly after bluetooth transmission), call set_solenoid_state, reset ticker to 0
    if (solenoid_ctl_received) {
      set_solenoid_state(solenoid_state);
      solenoid_state_timer = set_timer();
      solenoid_ctl_received = 0;

    // Otherwise, check for timeout of FLOW_STATE
    } else {
      if (solenoid_state == FLOW_STATE) {
        if (check_timer(solenoid_state_timer, solenoid_state_duration)) {
          set_solenoid_state(CUTOFF_STATE);
          solenoid_state = CUTOFF_STATE;
        }
      }
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

    // output to serial monitor
    //output_serial();
  }

  // send sensor info to controller
  if (check_timer(transmit_sens_timer, transmit_sens_interval)) {
    transmit_sens_timer = set_timer();

    // create json, serialize, and send to controller
    transmit_sensor();

    n_readings = 0;
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

int read_packet() {

  char rx_buffer[PACKET_BUFSIZ];
  char c;
  int nchar = 0;

  sprintf(err_msg, "OK");

  while (Serial.available()) {
    c = Serial.read();
    if (c != '\n') {
      rx_buffer[nchar] = c;
      nchar++;
    }
  }
  rx_buffer[nchar] = '\0';
  
  if (nchar == 0) {
    sprintf(err_msg, "Failed to read");
    return 1;
  }

  Serial.println(rx_buffer); // debug

  // Deserialize the JSON document
  StaticJsonDocument<100> doc;
  DeserializationError error = deserializeJson(doc, rx_buffer);

  // Test if parsing succeeds.
  if (error) {
    sprintf(err_msg, "deserializeJson() failed: %s", error.f_str());
    return 2;
  }

  // check transmission ID
  curr_rxpct = doc["packet number"];
  if (!((curr_rxpct == prev_rxpct+1) || (curr_rxpct == 0 && prev_rxpct == 99))) {
    sprintf(err_msg, "BT Error: packet dropped. Previous transmission received: #%d, Transmission just received: #%d", prev_rxpct, curr_rxpct);
  }

  // Update solenoid ctl
  bool rx_solenoid_state = doc["solenoid state"];
  if (rx_solenoid_state == 0 || rx_solenoid_state == 1) {
    solenoid_state = rx_solenoid_state;
  } else {
    sprintf(err_msg, "BT Error: Invalid solenoid ctl received");
    return 3;
  }

  if (solenoid_state) {
    int new_duration = doc["solenoid on duration"];
    solenoid_state_duration = ((new_duration + solenoid_ctl_interval/2) / solenoid_ctl_interval) * solenoid_ctl_interval; // should round it to nearest multiple of solenoid_ctl_interval
  }

  solenoid_ctl_received = 1;
  prev_rxpct = curr_rxpct;

  return 0;
}

void increment_Ntranspacket() {
  Ntxpct++;

  // Keep under 100 so we only take up 2 chars in packet
  if (Ntxpct >= 100) {
    Ntxpct = 0;
  }
}

void transmit_ack() {

  StaticJsonDocument<100> doc;

  doc["id"] = MYID;
  doc["packet number"] = Ntxpct;
  doc["transmission type"] = "Ack";
  doc["message"] = err_msg;

  char tx_ack[PACKET_BUFSIZ];
  serializeJson(doc, tx_ack);
  Serial.println(tx_ack);

  increment_Ntranspacket();
}

void transmit_sensor() {

  StaticJsonDocument<100> doc;

  doc["id"] = MYID;
  doc["packet number"] = Ntxpct;
  doc["transmission type"] = "Sensor Data";
  doc["moisture"] = avg_moist;
  doc["humidity"] = avg_hum;
  doc["temperature"] = avg_temp;
  doc["ir"] = avg_ir;
  doc["vis"] = avg_vis;
  doc["uv"] = avg_uv;
  doc["ph"] = avg_ph;

  char tx_packet[PACKET_BUFSIZ];
  serializeJson(doc, tx_packet);
  Serial.println(tx_packet);

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

float update_avg(float new_value, float avg_value, int n_readings) {
  return (avg_value*(float)(n_readings - 1) + new_value)/(float)n_readings;
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
  Serial.print(" lum\t pH: ");
  Serial.println(ph);
}

// Outputs averaged sensor values over transmit_sens_interval to serial monitor
void output_serial_transmission() {
  Serial.println("........TRANSMITTING:................");
  Serial.print("Moisture: ");
  Serial.print(avg_moist);
  Serial.print("%\t Humidity: ");
  Serial.print(avg_hum);
  Serial.print("%\t Temperature: ");
  Serial.print(avg_temp);
  Serial.print(" F\t IR: ");
  Serial.print(avg_ir);
  Serial.print(" lum\t Visible: ");
  Serial.print(avg_vis);
  Serial.print(" lum\t UV: ");
  Serial.print(avg_uv);
  Serial.print(" lum\t pH: ");
  Serial.println(avg_ph);
  Serial.println("......................................");
}

/*    ###############################   SOILENOID CONTROLS  #########################   */

// turns solenoid off or on
void set_solenoid_state(bool state) {
  if (state == CUTOFF_STATE) {
    digitalWrite(SOLENOIDPIN, LOW);
  } else {
    digitalWrite(SOLENOIDPIN, HIGH);
  }
}
