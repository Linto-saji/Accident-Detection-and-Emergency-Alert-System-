/*
 * Accident Detection & Emergency Alert System
 */

#include <Wire.h>

/* ================= CONFIG ================= */

#define MPU6050_ADDR 0x68

#define BUZZER_PIN 8
#define LED_PIN 13
#define BUTTON_PIN 2

#define IMPACT_THRESHOLD 12.0      // m/s² (demo value)
#define ALERT_CANCEL_TIME 10000    // 10 seconds
#define PRINT_INTERVAL 1000        // 1 second

const char PHONE_NUMBER[] = "+917907342255";  // Change number here

/* ================= VARIABLES ================= */

bool accidentDetected = false;
bool waitingForCancel = false;
bool alertSent = false;

unsigned long alertStartTime = 0;
unsigned long lastPrintTime = 0;

// Button edge detection
bool buttonPrevState = HIGH;
bool cancelRequest = false;

// Countdown
int lastSecondsRemaining = -1;

// Simulated GPS
float latitude = 40.712776;
float longitude = -74.005974;

/* ================= MPU6050 LOW LEVEL ================= */

void mpuWrite(byte reg, byte data) {
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(reg);
  Wire.write(data);
  Wire.endTransmission();
}

void mpuRead(byte reg, byte count, byte* data) {
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU6050_ADDR, count, true);
  for (byte i = 0; i < count; i++) {
    data[i] = Wire.read();
  }
}

void initMPU6050() {
  mpuWrite(0x6B, 0x00);   // Wake up
  mpuWrite(0x1C, 0x10);   // ±8g range
}

/* ================= SETUP ================= */

void setup() {
  Serial.begin(115200);
  Wire.begin();

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(LED_PIN, LOW);

  Serial.println("=== Accident Detection System ===");

  initMPU6050();
  Serial.println("MPU6050 Ready");
  Serial.println("State: MONITORING\n");
}

/* ================= LOOP ================= */

void loop() {

  /* ===== BUTTON EDGE DETECTION ===== */
  bool buttonNow = digitalRead(BUTTON_PIN);
  if (buttonPrevState == HIGH && buttonNow == LOW) {
    delay(30); // debounce
    if (digitalRead(BUTTON_PIN) == LOW) {
      cancelRequest = true;
    }
  }
  buttonPrevState = buttonNow;

  /* ===== READ MPU6050 ===== */
  byte rawData[6];
  mpuRead(0x3B, 6, rawData);

  int16_t axRaw = (rawData[0] << 8) | rawData[1];
  int16_t ayRaw = (rawData[2] << 8) | rawData[3];
  int16_t azRaw = (rawData[4] << 8) | rawData[5];

  float ax = (axRaw / 4096.0) * 9.8;
  float ay = (ayRaw / 4096.0) * 9.8;
  float az = (azRaw / 4096.0) * 9.8 - 9.8;

  float magnitude = sqrt(ax * ax + ay * ay + az * az);

  /* ===== CLEAN SERIAL OUTPUT ===== */
  if (!accidentDetected && millis() - lastPrintTime >= PRINT_INTERVAL) {
    Serial.print("ACC:");
    Serial.print(ax, 2);
    Serial.print(",");
    Serial.print(ay, 2);
    Serial.print(",");
    Serial.print(az, 2);
    Serial.print(" | MAG:");
    Serial.print(magnitude, 2);
    Serial.print(" | GPS:");
    Serial.print(latitude, 6);
    Serial.print(",");
    Serial.println(longitude, 6);

    latitude += random(-5, 5) / 1000000.0;
    longitude += random(-5, 5) / 1000000.0;

    lastPrintTime = millis();
  }

  /* ===== ACCIDENT DETECTION ===== */
  if (!accidentDetected && magnitude > IMPACT_THRESHOLD) {
    accidentDetected = true;
    waitingForCancel = true;
    alertSent = false;
    alertStartTime = millis();
    lastSecondsRemaining = -1;

    Serial.println("\n⚠️ HIGH DECELERATION DETECTED");
    Serial.println("State: ALERT_PENDING");
    Serial.println("Press button to cancel (10s)");
  }

  /* ===== ALERT CANCEL WINDOW ===== */
  if (waitingForCancel) {

    bool blink = millis() % 400 < 200;
    digitalWrite(LED_PIN, blink);
    digitalWrite(BUZZER_PIN, blink);

    unsigned long elapsed = millis() - alertStartTime;
    int secondsRemaining = (ALERT_CANCEL_TIME - elapsed) / 1000;

    if (secondsRemaining != lastSecondsRemaining && secondsRemaining >= 0) {
      Serial.print("Countdown: ");
      Serial.print(secondsRemaining);
      Serial.println(" sec");
      lastSecondsRemaining = secondsRemaining;
    }

    // User cancels alert
    if (cancelRequest) {
      cancelRequest = false;
      resetSystem();
      Serial.println("Alert cancelled (Hard braking)");
    }

    // Timeout → send alert
    if (!alertSent && elapsed >= ALERT_CANCEL_TIME) {
      alertSent = true;
      waitingForCancel = false;
      digitalWrite(BUZZER_PIN, LOW);
      digitalWrite(LED_PIN, HIGH);
      sendEmergencyAlert();
    }
  }
}

/* ================= SUPPORT ================= */

void resetSystem() {
  accidentDetected = false;
  waitingForCancel = false;
  alertSent = false;

  digitalWrite(LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  Serial.println("State: MONITORING\n");
}

void sendEmergencyAlert() {
  Serial.println("\n==============================");
  Serial.println("🚨 EMERGENCY ALERT");
  Serial.print("To: ");
  Serial.println(PHONE_NUMBER);
  Serial.println("Message: Accident detected!");
  Serial.print("Location: ");
  Serial.print(latitude, 6);
  Serial.print(",");
  Serial.println(longitude, 6);
  Serial.println("==============================");
}
