#include "Freenove_4WD_Car_for_Arduino.h"
#include "RF24_Remote.h"

#include <Servo.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

Servo GizmoServo;
Adafruit_MPU6050 mpu;

#define GIZMO_SERVO_PIN 3
#define AUTO_BUTTON_PIN 4
#define BLUE_LED_PIN 12
#define RED_LED_PIN 13

bool autoModeActive = false;
unsigned long autoStartTime = 0;
const unsigned long autoDuration = 10000; // 10 seconds

u32 lastNrfUpdateTime = 0;
#define NRF_UPDATE_TIMEOUT 1000

// -------------------------------------------------------------------

void setup() {
  pinsSetup();

  if (!nrf24L01Setup()) {
    alarm(3, 2);  // 3 short, 2 long beeps = NRF failure
  }

  alarm(2, 1);  // Normal startup indication

  // --- Gizmo Servo ---
  GizmoServo.attach(GIZMO_SERVO_PIN);
  GizmoServo.write(90);

  // --- Auto Button and LEDs ---
  pinMode(AUTO_BUTTON_PIN, INPUT_PULLUP);
  pinMode(BLUE_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);

  // --- MPU6050 Setup ---
  Wire.begin();
  if (!mpu.begin()) {
    Serial.begin(115200);
    Serial.println("Failed to find MPU6050 chip!");
    while (1) delay(10);
  }

  mpu.setAccelerometerRange(MPU6050_RANGE_16_G);
  mpu.setGyroRange(MPU6050_RANGE_250_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  Serial.println("MPU6050 ready.\n");
  delay(100);
}

// -------------------------------------------------------------------

void loop() {
  // --- AUTO MODE BUTTON (Local Trigger) ---
  if (!digitalRead(AUTO_BUTTON_PIN) && !autoModeActive) {
    autoModeActive = true;
    autoStartTime = millis();
    Serial.println("Auto mode activated!");
  }

  // --- AUTO MODE BEHAVIOR ---
  if (autoModeActive) {
    unsigned long elapsed = millis() - autoStartTime;

    // Move forward-left slightly
    carRun(FORWARD, 180);   // drive forward
    motorLeft(SPEED_LEVEL_4);   // slightly higher left motor
    motorRight(SPEED_LEVEL_3);  // slightly slower right motor

    // Read gyro for info/debug
    sensors_event_t a, g, temp; //acceleration, gyro, temp
    mpu.getEvent(&a, &g, &temp);
    Serial.print("Gyro Z: ");
    Serial.println(g.gyro.z);

    // End auto mode after duration
    if (elapsed >= autoDuration) {
      autoModeActive = false;
      carStop();
      Serial.println("Auto mode complete.");
    }

    delay(50);
    return;  // Skip RF control while in auto mode
  }

  // --- NORMAL RF CONTROL MODE ---
  if (getNrf24L01Data()) {
    clearNrfFlag();
    updateCarActionByNrfRemote();
    lastNrfUpdateTime = millis();
  }

  // --- FAILSAFE ---
  if (millis() - lastNrfUpdateTime > NRF_UPDATE_TIMEOUT) {
    lastNrfUpdateTime = millis();
    resetNrfDataBuf();
    updateCarActionByNrfRemote();
  }

  // --- GIZMO SERVO (OPTIONAL EXAMPLE) ---
  // You can drive it from RF signals or local logic
  // Example: toggle with a button
  static bool gizmoLeft = false;
  if (remoteData.btnA == 1) {
    gizmoLeft = !gizmoLeft;
    GizmoServo.write(gizmoLeft ? 45 : 135);
  }
}

// -------------------------------------------------------------------

