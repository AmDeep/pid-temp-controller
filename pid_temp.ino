#include <OneWire.h>
#include <DallasTemperature.h>

const int ONE_WIRE_BUS = 2;
const int HEATER_PIN   = 9;

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

float setpoint = 40.0;
float Kp = 8.0;
float Ki = 0.4;
float Kd = 15.0;

float integral = 0.0;
float prevError = 0.0;
float lastTemp = NAN;
unsigned long lastTime = 0;
const unsigned long SAMPLE_MS = 1000;
bool heaterEnabled = true;
uint32_t sampleCount = 0;
uint32_t sensorErrors = 0;

void printHelp() {
  Serial.println(F("Commands:"));
  Serial.println(F("  Sxx.x   set setpoint (C)"));
  Serial.println(F("  Pxx.x   set Kp"));
  Serial.println(F("  Ixx.x   set Ki"));
  Serial.println(F("  Dxx.x   set Kd"));
  Serial.println(F("  E0/E1   disable/enable heater"));
  Serial.println(F("  ?       help"));
}

void setup() {
  pinMode(HEATER_PIN, OUTPUT);
  analogWrite(HEATER_PIN, 0);
  Serial.begin(9600);
  sensors.begin();
  sensors.setResolution(12);
  Serial.println(F("PID temperature controller"));
  printHelp();
  lastTime = millis();
}

void handleSerial() {
  if (!Serial.available()) return;
  char cmd = Serial.read();
  if (cmd == 'S' || cmd == 's') {
    float v = Serial.parseFloat();
    if (v > 0 && v < 120) {
      setpoint = v;
      integral = 0;
      Serial.print(F("Setpoint=")); Serial.println(setpoint);
    }
  } else if (cmd == 'P' || cmd == 'p') {
    float v = Serial.parseFloat();
    if (v >= 0) { Kp = v; Serial.print(F("Kp=")); Serial.println(Kp); }
  } else if (cmd == 'I' || cmd == 'i') {
    float v = Serial.parseFloat();
    if (v >= 0) { Ki = v; integral = 0; Serial.print(F("Ki=")); Serial.println(Ki); }
  } else if (cmd == 'D' || cmd == 'd') {
    float v = Serial.parseFloat();
    if (v >= 0) { Kd = v; Serial.print(F("Kd=")); Serial.println(Kd); }
  } else if (cmd == 'E' || cmd == 'e') {
    int v = Serial.parseInt();
    heaterEnabled = (v != 0);
    if (!heaterEnabled) analogWrite(HEATER_PIN, 0);
    Serial.print(F("Heater ")); Serial.println(heaterEnabled ? F("enabled") : F("disabled"));
  } else if (cmd == '?') {
    printHelp();
  }
  while (Serial.available()) Serial.read();
}

void loop() {
  handleSerial();

  unsigned long now = millis();
  if (now - lastTime < SAMPLE_MS) return;
  float dt = (now - lastTime) / 1000.0f;
  lastTime = now;
  sampleCount++;

  sensors.requestTemperatures();
  float temp = sensors.getTempCByIndex(0);

  if (temp == DEVICE_DISCONNECTED_C || temp < -50 || temp > 150) {
    sensorErrors++;
    Serial.println(F("Sensor error - heater off"));
    analogWrite(HEATER_PIN, 0);
    return;
  }
  lastTemp = temp;

  if (!heaterEnabled) {
    Serial.print(F("T=")); Serial.print(temp, 2);
    Serial.print(F(" SP=")); Serial.print(setpoint, 1);
    Serial.println(F(" heater=off"));
    return;
  }

  float error = setpoint - temp;
  float pTerm = Kp * error;

  // integral with clamping (anti-windup)
  integral += error * dt;
  if (integral > 40.0f) integral = 40.0f;
  if (integral < -40.0f) integral = -40.0f;
  float iTerm = Ki * integral;

  float dTerm = 0;
  if (dt > 0.001f) {
    dTerm = Kd * (error - prevError) / dt;
  }
  prevError = error;

  float output = pTerm + iTerm + dTerm;
  if (output > 255.0f) output = 255.0f;
  if (output < 0.0f)   output = 0.0f;

  // conditional integration: freeze integral when saturated
  if ((output >= 255.0f && error > 0) || (output <= 0.0f && error < 0)) {
    integral -= error * dt; // undo the last integration step
  }

  analogWrite(HEATER_PIN, (int)output);

  Serial.print(F("T=")); Serial.print(temp, 2);
  Serial.print(F(" SP=")); Serial.print(setpoint, 1);
  Serial.print(F(" OUT=")); Serial.print(output, 0);
  Serial.print(F(" P=")); Serial.print(pTerm, 1);
  Serial.print(F(" I=")); Serial.print(iTerm, 1);
  Serial.print(F(" D=")); Serial.print(dTerm, 1);
  Serial.print(F(" n=")); Serial.println(sampleCount);
}
