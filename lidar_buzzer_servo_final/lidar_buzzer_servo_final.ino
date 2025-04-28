#include <SoftwareSerial.h>
#include <Servo.h>

// 🔹 LiDAR Configuration
SoftwareSerial Serial1(2, 3);  // LiDAR: Pin 2 (RX), Pin 3 (TX)
int dist;
int strength;
const int HEADER = 0x59;
const int distanceThreshold = 50;
const int strengthThreshold = 10;

// 🔹 Servo Configuration
Servo servo_9, servo_10, servo_11;

// 🔹 Speaker (via transistor)
const int speakerControl = 7;

// 🔹 LDR (Farm Light) Configuration
const int ldrPin = A0;
const int farmLight = 12;

// Timers
unsigned long buzzerStartTime = 0;
bool buzzerActive = false;
bool cooldownActive = false;
unsigned long cooldownStartTime = 0;
const unsigned long cooldownDuration = 180000;  // 3 min

// Invalid Data Speaker Timer
bool buzzerInvalidActive = false;
unsigned long buzzerInvalidStartTime = 0;
const unsigned long invalidBuzzerDuration = 30000;
bool invalidCooldownActive = false;
unsigned long invalidCooldownStartTime = 0;
const unsigned long invalidCooldownDuration = 120000;

// Servo Stop Timer for Invalid Data
bool servoStopActive = false;
unsigned long servoStopStartTime = 0;
const unsigned long servoStopDuration = 120000;

// ----------------- SETUP -----------------
void setup() {
  Serial.begin(9600);
  Serial1.begin(115200);
  delay(1000);

  servo_9.attach(9);
  servo_10.attach(10);
  servo_11.attach(11);

  pinMode(speakerControl, OUTPUT);
  pinMode(farmLight, OUTPUT);

  digitalWrite(speakerControl, LOW);
  digitalWrite(farmLight, LOW);

  Serial.println("✅ System Initialized...");
}

// ----------------- MAIN LOOP -----------------
void loop() {
  controlFarmLight();

  bool validData = readLidar();

  if (validData) {
    moveServos();
    handleSpeakerControl();
    cooldownActive = false;
    servoStopActive = false;
    buzzerInvalidActive = false;
    invalidCooldownActive = false;
  } else {
    handleInvalidSpeakerControl();
    handleInvalidServo();
  }

  delay(100);
}

// ----------------- LIDAR READING -----------------
bool readLidar() {
  static int uart[9];
  static int i, check;

  while (Serial1.available() >= 9) {
    if (Serial1.read() == HEADER) {
      uart[0] = HEADER;
      if (Serial1.read() == HEADER) {
        uart[1] = HEADER;
        for (i = 2; i < 9; i++) {
          uart[i] = Serial1.read();
        }

        check = 0;
        for (i = 0; i < 8; i++) check += uart[i];

        if (uart[8] == (check & 0xff)) {
          dist = uart[2] + uart[3] * 256;
          strength = uart[4] + uart[5] * 256;

          if (dist > 0 && dist < 12000 && strength > strengthThreshold) {
            Serial.print("📏 Distance = ");
            Serial.print(dist);
            Serial.print(" cm, 📡 Strength = ");
            Serial.println(strength);
            return true;
          } else {
            Serial.println("❌ Invalid Distance or Weak Signal ❌");
          }
        }
      }
    }
  }
  return false;
}

// ----------------- SPEAKER FOR VALID DATA -----------------
void handleSpeakerControl() {
  unsigned long currentTime = millis();

  if (dist <= distanceThreshold) {
    digitalWrite(speakerControl, LOW);
    buzzerActive = false;
    return;
  }

  if (cooldownActive) {
    if (currentTime - cooldownStartTime >= cooldownDuration) {
      cooldownActive = false;
      Serial.println("✅ Cooldown over, ready for next alert.");
    } else {
      Serial.println("⚠ Cooldown -> Speaker OFF.");
      return;
    }
  }

  if (!buzzerActive) {
    buzzerActive = true;
    buzzerStartTime = millis();
    digitalWrite(speakerControl, HIGH);
    Serial.println("🔊 Speaker ON (object detected)");
  }

  if (buzzerActive && (currentTime - buzzerStartTime >= 60000)) {
    digitalWrite(speakerControl, LOW);
    buzzerActive = false;
    Serial.println("⏳ 1 minute done -> Speaker OFF");

    cooldownActive = true;
    cooldownStartTime = millis();
    Serial.println("⏳ Cooldown started for 3 min");
  }

  Serial.print("🔍 Speaker Pin State: ");
  Serial.println(digitalRead(speakerControl));
}

// ----------------- SPEAKER FOR INVALID DATA -----------------
void handleInvalidSpeakerControl() {
  unsigned long currentTime = millis();

  if (invalidCooldownActive) {
    if (currentTime - invalidCooldownStartTime >= invalidCooldownDuration) {
      invalidCooldownActive = false;
      Serial.println("✅ Invalid data cooldown finished.");
    }
    return;
  }

  if (!buzzerInvalidActive) {
    buzzerInvalidActive = true;
    buzzerInvalidStartTime = millis();
    digitalWrite(speakerControl, HIGH);
    Serial.println("⚠ Invalid Data -> Speaker ON (30 sec)");
  }

  if (buzzerInvalidActive && (currentTime - buzzerInvalidStartTime >= invalidBuzzerDuration)) {
    digitalWrite(speakerControl, LOW);
    buzzerInvalidActive = false;
    invalidCooldownActive = true;
    invalidCooldownStartTime = millis();
    Serial.println("⏳ 30 sec done -> Speaker OFF");
    Serial.println("⏳ Speaker cooldown 2 min due to invalid data");
  }

  Serial.print("🔍 Speaker Pin State: ");
  Serial.println(digitalRead(speakerControl));
}

// ----------------- SERVO FOR INVALID DATA -----------------
void handleInvalidServo() {
  unsigned long currentTime = millis();

  if (!servoStopActive) {
    moveServos();
    servoStopActive = true;
    servoStopStartTime = millis();
    Serial.println("⚠ Invalid -> Servo moving 2 min");
  }

  if (servoStopActive && (currentTime - servoStopStartTime >= servoStopDuration)) {
    servoStopActive = false;
    Serial.println("✅ Servo reset after 2 min");
  }
}

// ----------------- SERVO MOVEMENT -----------------
void moveServos() {
  if (servoStopActive) return;

  for (int pos = 0; pos <= 100; pos++) {
    servo_9.write(pos);
    delay(15);
  }
  for (int pos = 100; pos >= 0; pos--) {
    servo_9.write(pos);
    delay(15);
  }

  delay(2000);

  for (int pos = 0; pos <= 100; pos++) {
    servo_10.write(pos);
    delay(15);
  }
  for (int pos = 100; pos >= 0; pos--) {
    servo_10.write(pos);
    delay(15);
  }

  delay(3000);

  for (int pos = 0; pos <= 100; pos++) {
    servo_11.write(pos);
    delay(50);
  }
  for (int pos = 100; pos >= 0; pos--) {
    servo_11.write(pos);
    delay(50);
  }
}

// ----------------- FARM LIGHT CONTROL USING LDR -----------------
void controlFarmLight() {
  int ldrState = digitalRead(ldrPin);

  if (ldrState == HIGH) {
    digitalWrite(farmLight, HIGH);
    Serial.println("🌙 Dark - Farm Light ON");
  } else {
    digitalWrite(farmLight, LOW);
    Serial.println("☀ Bright - Farm Light OFF");
  }
}