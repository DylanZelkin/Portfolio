#include <Servo.h>
#include <SoftwareSerial.h>

SoftwareSerial BTserial(13, 12); // TX, RX

Servo ESC1;
Servo ESC2;
Servo ESC3;
Servo ESC4;

bool esc1dir = false;
bool esc2dir = true;
bool esc3dir = false;
bool esc4dir = true;

int directions[4]; //altitude, roll, gas, turn

int motorThrottle[4]; //esc1 ...

void setThrottle(){

}


void setup() {
  Serial.begin(9600);
  BTserial.begin(9600);
  ESC1.attach(3,1000,2000);
  ESC2.attach(5,1000,2000);
  ESC3.attach(6,1000,2000);
  ESC4.attach(9,1000,2000);

  directions[0] = 90;
  directions[1] = 90;
  directions[2] = 90;
  directions[3] = 90;
}

void loop() {
  // Wait for data to be available on the Bluetooth serial port
  if (BTserial.available()) {
    // Read the data from the serial monitor and send it to the Bluetooth serial port
    String inputString = BTserial.readStringUntil('\n');
    // Serial.println("Recieving: " + inputString);

    //parse for directions
    String delimiter = ";";
    int i = 0;

    // Split the input string into tokens using ";" as the delimiter
    int pos = 0;
    String token;
    while ((pos = inputString.indexOf(delimiter)) != -1 && i < 4) {
      // Extract the token from the input string and remove it from the string
      token = inputString.substring(0, pos);
      inputString.remove(0, pos + delimiter.length());

      // Convert the token to an integer and store it in the values array
      directions[i] = token.toInt();
      i++;
    }

    // Parse the last token (after the last delimiter)
    if (i < 4) {
      directions[i] = inputString.toInt();
    }

    // Print the parsed integers
    // for (i = 0; i < 4; i++) {
    //   Serial.print("Value ");
    //   Serial.print(i);
    //   Serial.print(": ");
    //   Serial.println(directions[i]);
    // }

    //test: set 
    
  }
  ESC1.write(directions[0]);
  ESC2.write(directions[1]);
  ESC3.write(directions[2]);
  ESC4.write(directions[3]);
}
