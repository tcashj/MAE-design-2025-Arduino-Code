// -------------------- Libraries --------------------
#include <SPI.h>
#include "RF24.h"

// -------------------- NRF24L01 Configuration --------------------
RF24 radio(9, 10);                      // CE, CSN pins for the NRF24L01

// Each team should choose a unique address string.
// Example: "TM0xx" where xx = team number (e.g., TM001, TM014)
const byte addresses[6] = "TM013";       // Address must match receiver

// Each team should choose a unique channel between 20 and 127.
// Formula suggestion: (TeamNumber * 5) + 15
const int RF24channel = 80;              // Must match receiver

// Array to hold 8 data values (2 pots, 2 joystick axes, 4 switches)
int dataWrite[8];

// -------------------- Pin Assignments --------------------
const int pot1Pin = A0;          // POT1
const int pot2Pin = A1;          // POT2
const int joystickXPin = A2;     // Joystick X-axis
const int joystickYPin = A3;     // Joystick Y-axis
const int joystickZPin = 7;      // Joystick button (Z)
const int s1Pin = 4;             // Switch 1
const int s2Pin = 3;             // Switch 2
const int s3Pin = 2;             // Switch 3
const int led1Pin = 6;           // LED1 (next to POT1)
const int led2Pin = 5;           // LED2 (next to POT2)
const int led3Pin = 8;           // LED3 (next to NRF24L01)

// -------------------- Setup --------------------
void setup() {
  // Initialize NRF24L01
  radio.begin();
  radio.setPALevel(RF24_PA_MAX);      // Maximum power level
  radio.setDataRate(RF24_1MBPS);      // Data rate through the air
  radio.setRetries(10, 15);           // Retry delay and count
  radio.openWritingPipe(addresses);   // Open writing pipe
  radio.openReadingPipe(1, addresses);// Open reading pipe
  radio.setChannel(RF24channel);      // Set channel
  radio.stopListening();              // Transmitter mode

  // Configure input pins
  pinMode(joystickZPin, INPUT);
  pinMode(s1Pin, INPUT);
  pinMode(s2Pin, INPUT);
  pinMode(s3Pin, INPUT);

  // Configure output pins
  pinMode(led1Pin, OUTPUT);
  pinMode(led2Pin, OUTPUT);
  pinMode(led3Pin, OUTPUT);

  // Startup LED flash to confirm system init
  for (int i = 0; i < 3; i++) {
    digitalWrite(led1Pin, HIGH);
    digitalWrite(led2Pin, HIGH);
    digitalWrite(led3Pin, HIGH);
    delay(100);
    digitalWrite(led1Pin, LOW);
    digitalWrite(led2Pin, LOW);
    digitalWrite(led3Pin, LOW);
    delay(100);
  }
}

// -------------------- Main Loop --------------------
void loop() {
  // Read analog and digital inputs
  dataWrite[0] = analogRead(pot1Pin);       // Potentiometer 1
  dataWrite[1] = analogRead(pot2Pin);       // Potentiometer 2
  dataWrite[2] = analogRead(joystickXPin);  // Joystick X-axis
  dataWrite[3] = analogRead(joystickYPin);  // Joystick Y-axis
  dataWrite[4] = digitalRead(joystickZPin); // Joystick button
  dataWrite[5] = digitalRead(s1Pin);        // Switch 1
  dataWrite[6] = digitalRead(s2Pin);        // Switch 2
  dataWrite[7] = digitalRead(s3Pin);        // Switch 3

  // Attempt to send data packet
  if (radio.write(&dataWrite, sizeof(dataWrite))) {
    digitalWrite(led3Pin, HIGH);   // Transmission successful
  } else {
    digitalWrite(led3Pin, LOW);    // Transmission failed
    reinitRadio();                 // Reinitialize radio connection
  }

  // LED feedback for potentiometers
  analogWrite(led1Pin, map(dataWrite[0], 0, 1023, 0, 255));
  analogWrite(led2Pin, map(dataWrite[1], 0, 1023, 0, 255));

  delay(20); // 50Hz update rate (every 20ms)
}

// -------------------- Helper Functions --------------------
void reinitRadio() {
  // Reinitialize RF24 in case of connection loss
  radio.begin();
  radio.setPALevel(RF24_PA_MAX);
  radio.setDataRate(RF24_1MBPS);
  radio.setRetries(10, 15);
  radio.openWritingPipe(addresses);
  radio.openReadingPipe(1, addresses);
  radio.setChannel(RF24channel);
  radio.stopListening();
}
