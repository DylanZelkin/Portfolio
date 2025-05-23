#include <SoftwareSerial.h>

SoftwareSerial BTserial(13, 12); // TX, RX

void setup() {
  Serial.begin(9600);
  BTserial.begin(9600);
}

void loop() {
  // Wait for data to be available on the serial monitor
  if (Serial.available()) {
    // Read the data from the serial monitor and send it to the Bluetooth serial port
    String s = Serial.readStringUntil('\n');
    Serial.println("Sending: " + s);
    BTserial.println(s);
  }
}
