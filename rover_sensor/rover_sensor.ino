
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>

// ---------- Pins ----------
constexpr int PIN_VIB = 16;                 // D0, vibration module DO
constexpr int M1A = 14, M1B = 12;           // D5, D6 -> motor 1 inputs
constexpr int M2A = 13, M2B = 15;           // D7, D8 -> motor 2 inputs

// ---------- Settings ----------
constexpr int WINDOW_MS = 50;               // sensor sampling window
constexpr uint32_t MOTOR_TIMEOUT_MS = 2000; // auto-stop if no command (safety)

Adafruit_ADXL345_Unified accel(12345);
bool haveAccel = false;

int speedVal = 150;
bool moving = false;
uint32_t lastCmdMs = 0;

// ---------- Motors ----------
void setMotor(int a, int b, int s) {
  s = constrain(s, -255, 255);
  if (s >= 0) { analogWrite(a, s);  analogWrite(b, 0); }
  else        { analogWrite(a, 0);  analogWrite(b, -s); }
}

void drive(int l, int r) {
  setMotor(M1A, M1B, l);
  setMotor(M2A, M2B, r);
  moving = (l != 0 || r != 0);
  lastCmdMs = millis();
}

void handleCommand(char c) {
  switch (c) {
    case 'w': drive( speedVal,  speedVal); Serial.println("# forward");  break;
    case 's': drive(-speedVal, -speedVal); Serial.println("# back");     break;
    case 'a': drive(-speedVal,  speedVal); Serial.println("# left");     break;
    case 'd': drive( speedVal, -speedVal); Serial.println("# right");    break;
    case 'q': drive( speedVal, 0);         Serial.println("# motor1");   break;
    case 'e': drive(0, speedVal);          Serial.println("# motor2");   break;
    case 'x': drive(0, 0);                 Serial.println("# stop");     break;
    case '+': speedVal = min(255, speedVal + 10); Serial.printf("# speed=%d\n", speedVal); break;
    case '-': speedVal = max(0,   speedVal - 10); Serial.printf("# speed=%d\n", speedVal); break;
    default: break;   // ignore newline and other characters
  }
}

// ---------- I2C / ADXL345 ----------
void scanI2C() {
  Serial.println("# I2C scan:");
  int n = 0;
  for (uint8_t a = 1; a < 127; a++) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) {
      Serial.printf("#   found 0x%02X\n", a);
      n++;
    }
  }
  if (!n) Serial.println("#   nothing found - check SDA/SCL, 3V3, GND, solder joints");
}

bool tryAccel() {
  const uint8_t addrs[] = {0x53, 0x1D};   // 0x53 = SDO to GND, 0x1D = SDO to 3V3
  for (uint8_t a : addrs) {
    if (accel.begin(a)) {
      Serial.printf("# ADXL345 found at 0x%02X\n", a);
      accel.setRange(ADXL345_RANGE_4_G);
      accel.setDataRate(ADXL345_DATARATE_100_HZ);
      return true;
    }
  }
  return false;
}

// ---------- Setup ----------
void setup() {
  Serial.begin(115200);
  delay(200);

  pinMode(PIN_VIB, INPUT);

  pinMode(M1A, OUTPUT); pinMode(M1B, OUTPUT);
  pinMode(M2A, OUTPUT); pinMode(M2B, OUTPUT);
  analogWriteRange(255);
  analogWriteFreq(1000);
  drive(0, 0);

  Wire.begin(4, 5);   // SDA = D2, SCL = D1
  delay(100);

  scanI2C();
  haveAccel = tryAccel();
  if (!haveAccel) Serial.println("# ADXL345 not found - will keep retrying every 3 s");

  Serial.println("# Motor keys: w=fwd s=back a=left d=right q=motor1 e=motor2 x=stop +/- speed");
  Serial.println("# t_ms,ax,ay,az,amag,vib_state,vib_edges,sound_pp");
}

// ---------- Loop ----------
void loop() {
  // 1. Motor commands from the Serial Monitor
  while (Serial.available()) handleCommand(Serial.read());

  // 2. Safety: stop motors if no command for a while
  if (moving && millis() - lastCmdMs > MOTOR_TIMEOUT_MS) {
    drive(0, 0);
    Serial.println("# motor timeout - stopped");
  }

  // 3. Retry the accelerometer every 3 seconds if it was not found
  static uint32_t lastRetry = 0;
  if (!haveAccel && millis() - lastRetry > 3000) {
    lastRetry = millis();
    scanI2C();
    haveAccel = tryAccel();
    if (!haveAccel) Serial.println("# ADXL345 still not found");
  }

  // 4. Sample sound (A0) and vibration (D0) over one window
  int snMin = 1023, snMax = 0;
  int lastVib = digitalRead(PIN_VIB);
  int vibEdges = 0;
  uint32_t t0 = millis();
  while (millis() - t0 < WINDOW_MS) {
    int s = analogRead(A0);
    if (s < snMin) snMin = s;
    if (s > snMax) snMax = s;
    int v = digitalRead(PIN_VIB);
    if (v != lastVib) { vibEdges++; lastVib = v; }
    yield();
  }

  // 5. Read accelerometer (m/s^2)
  float ax = 0, ay = 0, az = 0, mag = 0;
  if (haveAccel) {
    sensors_event_t e;
    accel.getEvent(&e);
    ax = e.acceleration.x;
    ay = e.acceleration.y;
    az = e.acceleration.z;
    mag = sqrtf(ax * ax + ay * ay + az * az);
  }

  // 6. Print one CSV line
  Serial.printf("%lu,%.2f,%.2f,%.2f,%.2f,%d,%d,%d\n",
                millis(), ax, ay, az, mag, lastVib, vibEdges, snMax - snMin);
}