// =====================================================
// STABLE T-SHIRT CANNON CONTROL CODE
// Fixed relay flicker / multi-switch glitching
// Keeps original wiring
// Arduino Mega
// =====================================================

// ================== INPUT PINS ==================
#define CH3_PIN 6    // Forward/back
#define CH4_PIN 7    // Turning

#define CH7_PIN 9    // Relay controls
#define CH8_PIN 11
#define CH9_PIN 12
#define CH10_PIN 13

// ================== RELAYS ==================
#define RELAY6 28
#define RELAY5 30
#define RELAY4 32
#define RELAY3 34

// ================== MOTOR DRIVER ==================
#define ENA 4
#define IN1 29
#define IN2 31

#define ENB 5
#define IN3 33
#define IN4 35

// ================== RC SETTINGS ==================
#define CENTER3 1468
#define CENTER4 1480

#define DEADZONE 0.08

// Relay hysteresis
#define RELAY_ON   1550
#define RELAY_OFF  1450

// ================== CONTROL INVERTS ==================
#define INVERT_FORWARD true
#define INVERT_TURN    true
#define INVERT_LEFT    false
#define INVERT_RIGHT   false

// ================== GLOBALS ==================

// Last known good movement signals
int ch3 = CENTER3;
int ch4 = CENTER4;

// Last known good relay signals
int ch7 = 1000;
int ch8 = 1000;
int ch9 = 1000;
int ch10 = 1000;

// Relay states
bool relayState[4] = {false, false, false, false};

// Debounce timers
unsigned long relayTimer[4] = {0, 0, 0, 0};

// ================== SETUP ==================
void setup() {

  Serial.begin(115200);

  // Receiver inputs
  pinMode(CH3_PIN, INPUT);
  pinMode(CH4_PIN, INPUT);

  pinMode(CH7_PIN, INPUT);
  pinMode(CH8_PIN, INPUT);
  pinMode(CH9_PIN, INPUT);
  pinMode(CH10_PIN, INPUT);

  // Motor driver outputs
  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);

  pinMode(ENB, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  // Relay outputs
  pinMode(RELAY6, OUTPUT);
  pinMode(RELAY5, OUTPUT);
  pinMode(RELAY4, OUTPUT);
  pinMode(RELAY3, OUTPUT);

  // Start relays OFF
  digitalWrite(RELAY6, LOW);
  digitalWrite(RELAY5, LOW);
  digitalWrite(RELAY4, LOW);
  digitalWrite(RELAY3, LOW);
}

// =====================================================
// READ RC CHANNEL SAFELY
// Returns last valid value if bad signal occurs
// =====================================================
int readChannel(byte pin, int lastValue) {

  // Reduced timeout helps stability
  int pulse = pulseIn(pin, HIGH, 18000);

  // Accept only real RC pulse widths
  if (pulse >= 900 && pulse <= 2100) {
    return pulse;
  }

  // Bad read -> keep old value
  return lastValue;
}

// =====================================================
// MOTOR CONTROL
// =====================================================
void setMotor(int pwmPin, int in1, int in2, int speed, bool invert) {

  if (invert) speed = -speed;

  if (speed > 0) {

    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);

  }
  else if (speed < 0) {

    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);

  }
  else {

    digitalWrite(in1, LOW);
    digitalWrite(in2, LOW);
  }

  analogWrite(pwmPin, abs(speed));
}

// =====================================================
// STABLE RELAY CONTROL
// Includes:
// - hysteresis
// - debounce
// - bad signal protection
// =====================================================
void updateRelay(int signal, int relayPin, int index) {

  bool desiredState = relayState[index];

  // Hysteresis
  if (signal > RELAY_ON) {
    desiredState = true;
  }
  else if (signal < RELAY_OFF) {
    desiredState = false;
  }

  // Debounce / stability check
  if (desiredState != relayState[index]) {

    // Require stable state for 50ms
    if (millis() - relayTimer[index] > 50) {

      relayState[index] = desiredState;
      relayTimer[index] = millis();
    }
  }
  else {

    relayTimer[index] = millis();
  }

  digitalWrite(relayPin, relayState[index]);
}

// =====================================================
// MAIN LOOP
// =====================================================
void loop() {

  // ================== READ RECEIVER ==================

 ch7  = readChannel(CH7_PIN, ch7);
 ch8  = readChannel(CH8_PIN, ch8);
 ch9  = readChannel(CH9_PIN, ch9);
 ch10 = readChannel(CH10_PIN, ch10);

 ch3  = readChannel(CH3_PIN, ch3);
 ch4  = readChannel(CH4_PIN, ch4);

  // ================== MOVEMENT ==================

  float forward = (ch3 - CENTER3) / 500.0;
  float turn    = (ch4 - CENTER4) / 500.0;

  // Deadzone
  if (abs(forward) < DEADZONE) forward = 0;
  if (abs(turn) < DEADZONE) turn = 0;

  // Inversions
  if (INVERT_FORWARD) forward = -forward;
  if (INVERT_TURN) turn = -turn;

  // Tank steering mix
  float left  = forward + turn;
  float right = forward - turn;

  // Normalize
  float maxVal = max(abs(left), abs(right));

  if (maxVal > 1.0) {
    left /= maxVal;
    right /= maxVal;
  }

  // Convert to PWM
  int leftMotor  = left * 255;
  int rightMotor = right * 255;

  // Drive motors
  setMotor(ENA, IN1, IN2, leftMotor, INVERT_LEFT);
  setMotor(ENB, IN3, IN4, rightMotor, INVERT_RIGHT);

  // ================== RELAYS ==================

  updateRelay(ch7, RELAY6, 0);
  updateRelay(ch8, RELAY5, 1);
  updateRelay(ch9, RELAY4, 2);
  updateRelay(ch10, RELAY3, 3);

  // ================== DEBUG ==================

  Serial.print("CH7:");
  Serial.print(ch7);

  Serial.print(" CH8:");
  Serial.print(ch8);

  Serial.print(" CH9:");
  Serial.print(ch9);

  Serial.print(" CH10:");
  Serial.print(ch10);

  Serial.print(" || ");

  Serial.print(relayState[0]);
  Serial.print(" ");

  Serial.print(relayState[1]);
  Serial.print(" ");

  Serial.print(relayState[2]);
  Serial.print(" ");

  Serial.println(relayState[3]);

  // Faster loop
  delay(10);
}