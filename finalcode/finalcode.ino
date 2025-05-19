#include <SoftwareSerial.h>
#include <Servo.h>

// 🔹 LiDAR Configuration
SoftwareSerial Serial1(2, 3);  // LiDAR: Pin 2 (RX), Pin 3 (TX)
int dist;
int strength;
const int HEADER = 0x59;
const int strengthThreshold = 10;

// 🔹 Servo Configuration
Servo servo_9, servo_10, servo_11;

// 🔹 LDR (Farm Light) Configuration
const int ldrPin = A0;
const int farmLight = 12;

// 🔹 Megaphone Configuration
const int megaphoneControlPin = 7;
unsigned long lastMegaphoneTriggerTime = 0;
const unsigned long megaphoneCooldown = 60000; // 1 minute cooldown

// 🔹 Timers & Flags
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

  pinMode(farmLight, OUTPUT);
  digitalWrite(farmLight, LOW);

  pinMode(megaphoneControlPin, OUTPUT);
  digitalWrite(megaphoneControlPin, LOW);

  Serial.println("✅ System Initialized...");
}

// ----------------- MAIN LOOP -----------------
void loop() {
  controlFarmLight();

  bool validData = readLidar();

  if (validData) {
    moveServos();
    servoStopActive = false;
  } else {
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

            triggerMegaphone();
            return true;
          } else {
            Serial.println("❌ Invalid Distance or Weak Signal ❌");
            triggerMegaphone();  // 🔊 Trigger megaphone for invalid signal too
          }
        }
      }
    }
  }
  return false;
}

// ----------------- MEGAPHONE CONTROL -----------------
void triggerMegaphone() {
  unsigned long currentTime = millis();
  if (currentTime - lastMegaphoneTriggerTime >= megaphoneCooldown) {
    Serial.println("🔊 Megaphone Triggered");
    digitalWrite(megaphoneControlPin, HIGH);
    delay(5000);  // Simulate button hold
    digitalWrite(megaphoneControlPin, LOW);
    lastMegaphoneTriggerTime = currentTime;
  } else {
    Serial.println("⏳ Megaphone Cooldown Active");
  }
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


for (int pos = 0; pos <= 180; pos++) {
  servo_9.write(pos);
  delay(50);
}
for (int pos = 180; pos >= 0; pos--) {
  servo_9.write(pos);
  delay(50);
}


  delay(5000);

  for (int pos = 0; pos <= 100; pos++) {
    servo_10.write(pos);
    delay(50);
  }
  for (int pos = 100; pos >= 0; pos--) {
    servo_10.write(pos);
    delay(50);
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
    Serial.println("☀️ Bright - Farm Light OFF");
  }
}
