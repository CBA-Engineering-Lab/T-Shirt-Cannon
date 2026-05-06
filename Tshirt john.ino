// ================== INPUT PINS ==================
#define CH3_PIN 12   // Forward/back
#define CH4_PIN 13   // Turning
#define CH7_PIN 11   // Relay controls
#define CH8_PIN 10
#define CH9_PIN 9
#define CH10_PIN 8

// ================== RELAYS ==================
#define RELAY6 28
#define RELAY5 30
#define RELAY4 32
#define RELAY3 34

// Relay thresholds (with hysteresis to stop flicker)
#define RELAY_ON  1500
#define RELAY_OFF 1350

// ================== MOTOR DRIVER ==================
#define ENA 7
#define IN1 29
#define IN2 31

#define ENB 6
#define IN3 33
#define IN4 35

// ================== CONTROL SETTINGS ==================
#define CENTER3 1468
#define CENTER4 1480
#define DEADZONE 0.08   // Joystick deadzone (0–1 scale)

// Inversions (adjust if controls feel reversed)
#define INVERT_FORWARD true
#define INVERT_TURN    true
#define INVERT_LEFT    false
#define INVERT_RIGHT   false

// ================== GLOBAL STATE ==================
int ch3 = CENTER3;
int ch4 = CENTER4;

// Relay state memory (prevents flicker)
bool relayState[4] = {false, false, false, false};

// ================== SETUP ==================
void setup() {
  Serial.begin(115200);

  pinMode(CH3_PIN, INPUT);
  pinMode(CH4_PIN, INPUT);
  pinMode(CH7_PIN, INPUT);
  pinMode(CH8_PIN, INPUT);
  pinMode(CH9_PIN, INPUT);
  pinMode(CH10_PIN, INPUT);

  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  pinMode(RELAY6, OUTPUT);
  pinMode(RELAY5, OUTPUT);
  pinMode(RELAY4, OUTPUT);
  pinMode(RELAY3, OUTPUT);
}

// ================== MOTOR CONTROL ==================
void setMotor(int pwmPin, int in1, int in2, int speed, bool invert) {
  if (invert) speed = -speed;

  if (speed > 0) {
    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);
  } else if (speed < 0) {
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);
  } else {
    digitalWrite(in1, LOW);
    digitalWrite(in2, LOW);
  }

  analogWrite(pwmPin, abs(speed));
}

// ================== RELAY WITH HYSTERESIS ==================
void updateRelay(int signal, int pin, int index) {
  if (signal > RELAY_ON) {
    relayState[index] = true;
  } else if (signal < RELAY_OFF) {
    relayState[index] = false;
  }
  digitalWrite(pin, relayState[index]);
}

// ================== MAIN LOOP ==================
void loop() {

  // -------- READ RECEIVER SIGNALS --------
  int raw3 = pulseIn(CH3_PIN, HIGH, 30000);
  int raw4 = pulseIn(CH4_PIN, HIGH, 30000);

  int ch7 = pulseIn(CH7_PIN, HIGH, 30000);
  int ch8 = pulseIn(CH8_PIN, HIGH, 30000);
  int ch9 = pulseIn(CH9_PIN, HIGH, 30000);
  int ch10 = pulseIn(CH10_PIN, HIGH, 30000);

  // Keep last good value (prevents dropouts)
  if (raw3 >= 1000 && raw3 <= 2000) ch3 = raw3;
  if (raw4 >= 1000 && raw4 <= 2000) ch4 = raw4;

  // -------- MOTOR CALCULATIONS --------
  float forward = (ch3 - CENTER3) / 500.0;
  float turn    = (ch4 - CENTER4) / 500.0;

  // Deadzone
  if (abs(forward) < DEADZONE) forward = 0;
  if (abs(turn) < DEADZONE) turn = 0;

  // Inversions
  if (INVERT_FORWARD) forward = -forward;
  if (INVERT_TURN) turn = -turn;

  // Tank mix
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

  setMotor(ENA, IN1, IN2, leftMotor, INVERT_LEFT);
  setMotor(ENB, IN3, IN4, rightMotor, INVERT_RIGHT);

  // -------- RELAYS (STABLE NOW) --------
  updateRelay(ch7, RELAY6, 0);
  updateRelay(ch8, RELAY5, 1);
  updateRelay(ch9, RELAY4, 2);
  updateRelay(ch10, RELAY3, 3);

  // -------- DEBUG --------
  Serial.print("F: "); Serial.print(forward, 2);
  Serial.print(" T: "); Serial.print(turn, 2);
  Serial.print(" | L: "); Serial.print(leftMotor);
  Serial.print(" R: "); Serial.print(rightMotor);

  Serial.print(" || R7: "); Serial.print(relayState[0]);
  Serial.print(" R8: "); Serial.print(relayState[1]);
  Serial.print(" R9: "); Serial.print(relayState[2]);
  Serial.print(" R10: "); Serial.println(relayState[3]);

  delay(20); // Slightly faster loop
}