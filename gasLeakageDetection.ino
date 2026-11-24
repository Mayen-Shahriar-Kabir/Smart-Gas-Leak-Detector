#include <SoftwareSerial.h>
#include <Servo.h>

/*
 Smart Gas Leak Detection and Automatic Ventilation System

 Features:
 - Two MQ-2 gas sensors on A0 and A1
 - Buzzer alarm
 - Red LED for gas leak
 - Green LED for normal condition
 - Relay-controlled exhaust fan
 - Servo-controlled gas shut-off valve
 - SIM800L SMS alert

 Power:
 - Arduino, sensors, buzzer, LEDs from Power Bank 1
 - Servo, relay, fan, LM2596 from Power Bank 2
 - SIM800L from LM2596 adjusted to about 4.0 V
 - All GNDs must be common
 - Do NOT connect PB1 5V and PB2 5V together
*/

// ================= PIN DEFINITIONS =================

const byte MQ1_PIN = A0;
const byte MQ2_PIN = A1;

const byte SERVO_PIN = 9;
const byte RELAY_PIN = 8;
const byte BUZZER_PIN = 7;

const byte RED_LED_PIN = 6;
const byte GREEN_LED_PIN = 5;

// Arduino D2 receives from SIM800L TXD
// Arduino D3 transmits to SIM800L RXD through voltage divider
const byte GSM_RX_PIN = 2;
const byte GSM_TX_PIN = 3;

SoftwareSerial gsm(GSM_RX_PIN, GSM_TX_PIN);
Servo valveServo;

// ================= USER SETTINGS =================

// Replace with your phone number including country code
// Example Bangladesh: "+8801XXXXXXXXX"
const char PHONE_NUMBER[] = "+8801XXXXXXXXX";

// Servo angles
// Adjust these according to your gas valve mechanism
const int VALVE_OPEN_ANGLE = 0;
const int VALVE_CLOSED_ANGLE = 90;

// MQ sensor calibration settings
const int CALIBRATION_SAMPLES = 100;
const int READ_SAMPLES = 10;

// The threshold will be:
// threshold = clean_air_baseline + THRESHOLD_OFFSET
// Increase this if false alarms happen.
// Decrease this if gas is not detected.
const int THRESHOLD_OFFSET = 150;

// Hysteresis prevents fast ON/OFF flickering near threshold
const int HYSTERESIS = 50;

// Number of repeated gas readings required before alarm
const int LEAK_CONFIRM_COUNT = 3;

// Set true for safer behavior.
// Once gas leak is detected, alarm remains ON until Arduino reset.
const bool LATCH_ALARM_UNTIL_RESET = true;

// Many relay modules are active LOW.
// If your relay turns ON when Arduino output is LOW, set this to true.
// If your relay turns ON when Arduino output is HIGH, keep false.
const bool RELAY_ACTIVE_LOW = false;

// SMS settings
const unsigned long SMS_COOLDOWN_MS = 60000UL;  // 1 minute

// ================= GLOBAL VARIABLES =================

int baseline1 = 0;
int baseline2 = 0;

int threshold1 = 0;
int threshold2 = 0;

bool alarmLatched = false;
bool smsAlreadySent = false;

int leakCounter = 0;

unsigned long lastSmsTime = 0;

// ================= BASIC OUTPUT FUNCTIONS =================

void relayOn() {
 if (RELAY_ACTIVE_LOW) {
   digitalWrite(RELAY_PIN, LOW);
 } else {
   digitalWrite(RELAY_PIN, HIGH);
 }
}

void relayOff() {
 if (RELAY_ACTIVE_LOW) {
   digitalWrite(RELAY_PIN, HIGH);
 } else {
   digitalWrite(RELAY_PIN, LOW);
 }
}

void setNormalOutputs() {
 digitalWrite(GREEN_LED_PIN, HIGH);
 digitalWrite(RED_LED_PIN, LOW);
 digitalWrite(BUZZER_PIN, LOW);

 relayOff();
 valveServo.write(VALVE_OPEN_ANGLE);
}

void setLeakOutputs() {
 digitalWrite(GREEN_LED_PIN, LOW);
 digitalWrite(RED_LED_PIN, HIGH);

 // Beeping buzzer pattern
 if ((millis() / 300) % 2 == 0) {
   digitalWrite(BUZZER_PIN, HIGH);
 } else {
   digitalWrite(BUZZER_PIN, LOW);
 }

 relayOn();
 valveServo.write(VALVE_CLOSED_ANGLE);
}

// ================= SENSOR FUNCTIONS =================

int readAverageAnalog(byte pin) {
 long total = 0;

 for (int i = 0; i < READ_SAMPLES; i++) {
   total += analogRead(pin);
   delay(5);
 }

 return total / READ_SAMPLES;
}

void calibrateSensors() {
 Serial.println();
 Serial.println("Calibrating MQ sensors...");
 Serial.println("Keep sensors in clean air during calibration.");

 long total1 = 0;
 long total2 = 0;

 for (int i = 0; i < CALIBRATION_SAMPLES; i++) {
   total1 += analogRead(MQ1_PIN);
   total2 += analogRead(MQ2_PIN);

   delay(50);
 }

 baseline1 = total1 / CALIBRATION_SAMPLES;
 baseline2 = total2 / CALIBRATION_SAMPLES;

 threshold1 = baseline1 + THRESHOLD_OFFSET;
 threshold2 = baseline2 + THRESHOLD_OFFSET;

 Serial.println("Calibration complete.");
 Serial.print("MQ1 baseline: ");
 Serial.println(baseline1);
 Serial.print("MQ2 baseline: ");
 Serial.println(baseline2);
 Serial.print("MQ1 threshold: ");
 Serial.println(threshold1);
 Serial.print("MQ2 threshold: ");
 Serial.println(threshold2);
 Serial.println();
}

// ================= GSM FUNCTIONS =================

void sendATCommand(const char *cmd, unsigned long waitTime) {
 gsm.println(cmd);
 delay(waitTime);

 while (gsm.available()) {
   Serial.write(gsm.read());
 }
}

void initializeGSM() {
 Serial.println("Initializing SIM800L...");

 gsm.begin(9600);
 delay(2000);

 sendATCommand("AT", 1000);
 sendATCommand("ATE0", 1000);
 sendATCommand("AT+CMGF=1", 1000);       // SMS text mode
 sendATCommand("AT+CSCS=\"GSM\"", 1000); // GSM character set

 Serial.println("SIM800L initialization complete.");
}

void sendSMS(String message) {
 Serial.println("Sending SMS...");

 gsm.println("AT+CMGF=1");
 delay(500);

 gsm.print("AT+CMGS=\"");
 gsm.print(PHONE_NUMBER);
 gsm.println("\"");
 delay(1000);

 gsm.print(message);
 delay(500);

 // CTRL+Z ends SMS
 gsm.write(26);
 delay(5000);

 Serial.println("SMS command sent.");
}

// ================= SETUP =================

void setup() {
 Serial.begin(9600);

 pinMode(RELAY_PIN, OUTPUT);
 pinMode(BUZZER_PIN, OUTPUT);
 pinMode(RED_LED_PIN, OUTPUT);
 pinMode(GREEN_LED_PIN, OUTPUT);

 relayOff();
 digitalWrite(BUZZER_PIN, LOW);
 digitalWrite(RED_LED_PIN, LOW);
 digitalWrite(GREEN_LED_PIN, HIGH);

 valveServo.attach(SERVO_PIN);
 valveServo.write(VALVE_OPEN_ANGLE);

 Serial.println("Smart Gas Leak Detection System Starting...");
 Serial.println("Warming up sensors briefly...");

 delay(5000);

 calibrateSensors();
 initializeGSM();

 Serial.println("System ready.");
 Serial.println();
}

// ================= MAIN LOOP =================

void loop() {
 int mq1Value = readAverageAnalog(MQ1_PIN);
 int mq2Value = readAverageAnalog(MQ2_PIN);

 bool mq1High = mq1Value > threshold1;
 bool mq2High = mq2Value > threshold2;

 bool gasDetectedNow = mq1High || mq2High;

 if (gasDetectedNow) {
   leakCounter++;
 } else {
   if (leakCounter > 0) {
     leakCounter--;
   }
 }

 bool leakConfirmed = leakCounter >= LEAK_CONFIRM_COUNT;

 if (leakConfirmed) {
   alarmLatched = true;
 }

 bool alarmActive;

 if (LATCH_ALARM_UNTIL_RESET) {
   alarmActive = alarmLatched;
 } else {
   bool mq1Safe = mq1Value < (threshold1 - HYSTERESIS);
   bool mq2Safe = mq2Value < (threshold2 - HYSTERESIS);

   if (mq1Safe && mq2Safe) {
     alarmLatched = false;
     smsAlreadySent = false;
   }

   alarmActive = alarmLatched;
 }

 if (alarmActive) {
   setLeakOutputs();

   unsigned long now = millis();

   if (!smsAlreadySent || (now - lastSmsTime >= SMS_COOLDOWN_MS)) {
     String smsText = "WARNING! Gas leak detected. ";
     smsText += "Fan ON. Gas valve CLOSED. ";
     smsText += "MQ1=";
     smsText += mq1Value;
     smsText += ", MQ2=";
     smsText += mq2Value;

     sendSMS(smsText);

     smsAlreadySent = true;
     lastSmsTime = now;
   }
 } else {
   setNormalOutputs();
 }

 Serial.print("MQ1: ");
 Serial.print(mq1Value);
 Serial.print(" | MQ2: ");
 Serial.print(mq2Value);
 Serial.print(" | TH1: ");
 Serial.print(threshold1);
 Serial.print(" | TH2: ");
 Serial.print(threshold2);
 Serial.print(" | Leak counter: ");
 Serial.print(leakCounter);
 Serial.print(" | Alarm: ");

 if (alarmActive) {
   Serial.println("ON");
 } else {
   Serial.println("OFF");
 }

 delay(500);
}