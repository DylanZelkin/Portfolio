#include <SoftwareSerial.h>

//BT Connections: RX-> pin, TX -> pin, GND -> gnd, VCC -> 5V, EN -> 3.3V

SoftwareSerial Bluetooth(13, 12); // TX, RX

void setup() {
  Serial.begin(9600);
  Bluetooth.begin(38400);
} 

void loop() {
  if(Bluetooth.available()){
    Serial.write(Bluetooth.read());
  }

  if (Serial.available()) {
    Bluetooth.write(Serial.read());
  }
}


//Open Serial monitor and set output to have "Both NL & CR"
//For slave input:  AT+ROLE=0
//                  AT+ADDR?     //save this addr for master

//For master input: AT+ROLE=1
//                  AT+CMODE=0
//                  AT+BIND=addr(sub all : for ,)