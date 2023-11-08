#include <string.h>
#include <DHT.h>
#include <Wire.h>
#include "Si115X.h"
#include <SoftwareSerial.h>

/* Pinout:
- A0 -> moisture sensor output
- A1 -> pH sensor output
- 4  -> hum/temp sensor output
- 7  -> input to transistor that controls solenoid
- 8  -> bluetooth receive pin
- 9  -> bluetooth transmit pin
*/

#define RXPIN 8
#define TXPIN 9
#define REC_PACKET_LENGTH 32 // 1 byte packet ID, 28 bytes of sensor data, 1 byte solenoid ctl ack, 1 byte checksum, 1 byte stop_byte
#define TRANS_PACKET_LENGTH 6 // 1 byte packet ID, 1 byte solenoid state, 2 bytes ON length, 1 byte checksum, 1 byte stop_byte
#define STOP_BYTE '\0'
#define MOISTPIN 0
#define DRY_BOUND 523 // from calibration - soil moisture sensor output when just exposed to air - our humidity = 0%RH
#define WET_BOUND 254 // from calibration - soil moisture sensor output when in a cup of water - our humidity = 100%RH
#define PHPIN A1
#define DHTTYPE DHT22 // hum temp sensor type
#define DHTPIN 4
#define SOLENOIDPIN 7
#define FLOW_STATE 0
#define CUTOFF_STATE 1


// Initialize objects
SoftwareSerial BTSerial(RXPIN, TXPIN);
DHT dht(DHTPIN, DHTTYPE);
Si115X si1151;

// declare globals

char last_trans_ID_recieved;
char last_trans_ID_transmitted;

float moist, hum, temp, ir, vis, uv, ph;

// timing related:
unsigned long begin_time;
const unsigned long check_sens_interval = 1000; // in milliseconds
const unsigned long solenoid_ctl_interval = 100; // in milliseconds - we leave the water on for time in solenoid_control_interval increments

// solenoid control:
unsigned long solenoid_state_duration; // will be set upon receiving transmission from controller
bool solenoid_state;
bool solenoid_ctl_received;

void setup() {

  Serial.begin(9600); // start serial for output

  last_trans_ID_recieved = (char)0x7F;
  last_trans_ID_transmitted = (char)0x7F;

  pinMode(SOLENOIDPIN, OUTPUT);

  // Bluetooth HC-05
  BTSerial.begin(38400); // why this

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
}

void loop() {
  
  // check for bluetooth transmissions received
  // is it a problem if they arrive during delay()? -- no, there's a buffer. look into length of buffer
  // -- we think the buffer is 63 bytes, and FIFO
  // -- data is lost with a delay longer than 800 ms
  // if solenoid control message received, turn solenoid_ctl_received on, change vals of solenoid_state, and solenoid_state_duration if applic., set solenoid_state_ticker to 0, verify that solenoid_state_duration % solenoid_ctl_interval = 0 or round to make it so
  /*(if (BTSerial.available()) {
    char byte_read = BTSerial.read();
  }*/
  if (Serial.available()) {
    char msg = Serial.read();
    if (msg == 'f') {
      solenoid_state = FLOW_STATE;
      solenoid_ctl_received = 1;
      solenoid_state_duration = 3000;
    } else if (msg == 'c') {
      solenoid_state = CUTOFF_STATE;
      solenoid_ctl_received = 1;
    }
  }


  // SOLENOID CONTROL
  // check if it's been 1 solenoid_ctl_interval since the last time we accessed solenoid control
  if (time_reached(solenoid_ctl_interval)) {

    // Access solenoid control

    // If the state has changed via bluetooth instruction instead of timeout (done directly after bluetooth transmission), call set_solenoid_state, reset ticker to 0
    if (solenoid_ctl_received) {
      set_solenoid_state(solenoid_state);
      set_timer();
      solenoid_ctl_received = 0;

    // Otherwise, check for timeout of FLOW_STATE
    } else {
      if (solenoid_state == FLOW_STATE) {
        if (check_timer(solenoid_state_duration)) {
          set_solenoid_state(CUTOFF_STATE);
          solenoid_state = CUTOFF_STATE;
        }
      }
    }

  }
  
  // SENSORS
  // check if it's been 1 check_sens_interval since the last time we accessed the sensors
  if (time_reached(check_sens_interval)) {
    // update readings
    read_moist();
    read_hum_temp();
    read_sunlight();
    read_ph();

    // output to serial monitor
    output_serial();

    // ERROR CHECKING -- check for -1's
    // could also make temp sensor check for temp operational bounds for other sensors
  }

  // Bluetooth transmission
  // send acks
  // send sensor info
  //EEBlue.write();
}

/*    ###############################   TIMING     #########################   */
bool time_reached(unsigned long interval) {
  if ((millis() % interval) == 0) {
    return 1; // True
  }
  return 0;
}

void set_timer() {
  begin_time = millis();
}

bool check_timer(unsigned long duration) {
  if ((millis() - begin_time) >= duration) {
    return 1;
  }
  return 0;
}

/*    ###############################   BLUETOOTH  #########################   */

byte checksum(char *s, int length) {
  byte c = 0;
  for (int ic = 0; ic < length; ic++) {
    c^= *s++;
  }
  return c;
}

bool read_packet() {
  /*String buffer = BTSerial.readBytesUntil(STOP_BYTE);

  // check for incorrect length
  if (buffer.length() != REC_PACKET_LENGTH) {
    Serial.print("BT Error: packet received has unexpected length of ");
    Serial.println(buffer.length());
    return false;
  }

  // check checksum
  int checksum_index = REC_PACKET_LENGTH - 1;
  if (checksum(buffer.substring(0,checksum_index)) != buffer[checksum_index]) {
    Serial.println("BT Error: packet received checksum does not match");
    return false;
  }

  // check transmission ID
  byte trans_ID = buffer[0];
  if (!((trans_ID == (last_trans_id_received && 0x01)) || ((trans_ID == 0x01) && (last_trans_id_received == 0x7F)))) {
    Serial.print("BT Error: packet dropped. Previous transmission received: #");
    Serial.print(last_trans_id_received, HEX);
    Serial.print(", Transmission just received: #");
    Serial.print(trans_ID, HEX);
    return false;
  }

  // Update solenoid ctl
  char solenoid_state_indic = buffer[1];
  if (solenoid_state_indic = '0') {
    solenoid_state = 0;
  } else if (solenoid_state_indic = '1') {
    solenoid_state = 1;
  } else {
    Serial.println("BT Error: Invalid solenoid ctl received");
    return false;
  }

  int

  // verify that solenoid_state_duration % solenoid_ctl_interval = 0 or round to make it so

  solenoid_ctl_received = 1;
  solenoid_state_ticker = 0;*/

  return true;
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

/*    ###############################   SOILENOID CONTROLS  #########################   */

// turns solenoid off or on
void set_solenoid_state(bool state) {
  if (state == CUTOFF_STATE) {
    //Serial.println("solenoid off!");
    digitalWrite(SOLENOIDPIN, HIGH);
  } else {
    //Serial.println("solenoid on!");
    digitalWrite(SOLENOIDPIN, LOW);
  }
}
