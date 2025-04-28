#include <SoftwareSerial.h>    // For LiDAR communication
#include <Servo.h>              // For controlling servo motors

// LiDAR Configuration
SoftwareSerial Serial1(2, 3);   // LiDAR: Pin 2 = RX, Pin 3 = TX
int dist;                       // Distance from LiDAR
int strength;                   // Signal strength
int check;
int i;
int uart[9];                     // Array to store LiDAR data
const int HEADER = 0x59;        // LiDAR frame header

// Servo Motor Configuration
Servo servo_9;                   // Servo 1
Servo servo_10;                  // Servo 2
Servo servo_11;                  // Servo 3
int pos = 0;                     

// Buzzer Configuration
const int buzzer = 8;            // Buzzer connected to pin 8
const int distanceThreshold = 100; // Threshold distance to trigger buzzer

void setup() {
  Serial.begin(9600);            // Serial monitor baud rate
  Serial1.begin(115200);         // LiDAR communication baud rate
  delay(1000);                   // Delay for stability

  // Attach servos to pins
  servo_9.attach(9, 500, 2500);  
  servo_10.attach(10, 500, 2500); 
  servo_11.attach(11, 500, 2500); 

  pinMode(buzzer, OUTPUT);       // Set buzzer pin as output
  digitalWrite(buzzer, LOW);     // Ensure buzzer is off initially

  Serial.println("System Initialized...");
}

void loop() {
  readLidar();    // Read LiDAR data
  moveServos();   // Perform servo movements

  Serial.println("Waiting 15 seconds...");
  delay(15000);   // 15-second delay after completing all movements
}

// --- LiDAR Reading Function ---
void readLidar() {
  while (Serial1.available() >= 9) {      // Ensure at least 9 bytes are available
    if (Serial1.read() == HEADER) {
      uart[0] = HEADER;
      
      if (Serial1.read() == HEADER) {
        uart[1] = HEADER;

        for (i = 2; i < 9; i++) {         // Read remaining bytes
          uart[i] = Serial1.read();
        }

        // Verify checksum
        check = uart[0] + uart[1] + uart[2] + uart[3] + uart[4] + uart[5] + uart[6] + uart[7];
        
        if (uart[8] == (check & 0xff)) {   // Validate checksum
          dist = uart[2] + uart[3] * 256;     
          strength = uart[4] + uart[5] * 256; 

          if (dist > 0 && dist < 12000) {   // Filter invalid readings
            Serial.print("Distance = ");
            Serial.print(dist);
            Serial.print(" cm\t");
            Serial.print("Strength = ");
            Serial.println(strength);

            if (dist < distanceThreshold) { // Trigger buzzer if distance is below threshold
              digitalWrite(buzzer, HIGH);
            } else {
              digitalWrite(buzzer, LOW);
            }
          } else {
            Serial.println("Invalid distance reading.");
          }
        }
      }
    }
  }
}

// --- Servo Motor Movements ---
void moveServos() {
  // Servo 1: Move from 0° to 100° and back
  for (pos = 0; pos <= 100; pos += 1) {
    servo_9.write(pos);
    delay(15);
  }
  for (pos = 100; pos >= 0; pos -= 1) {
    servo_9.write(pos);
    delay(15);
  }
  
  delay(2000);  // Short delay between movements

  // Servo 2: Move from 0° to 100° and back
  for (pos = 0; pos <= 100; pos += 1) {
    servo_10.write(pos);
    delay(15);
  }
  for (pos = 100; pos >= 0; pos -= 1) {
    servo_10.write(pos);
    delay(15);
  }
  
  delay(3000);  // Short delay between movements

  // Servo 3: Move from 0° to 100° and back
  for (pos = 0; pos <= 100; pos += 1) {
    servo_11.write(pos);
    delay(50);
  }
  for (pos = 100; pos >= 0; pos -= 1) {
    servo_11.write(pos);
    delay(50);
  }
}