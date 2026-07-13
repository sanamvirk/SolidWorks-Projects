#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define SS_PIN 10
#define RST_PIN 9
#define MOTOR_PIN 5

#define TRIG_PIN 6
#define ECHO_PIN 7

MFRC522 myRFID(SS_PIN, RST_PIN);
LiquidCrystal_I2C lcd(0x27, 16, 2);  // change to 0x3F if needed

const String dog1UID = "49 27 A9 00";
const String dog2UID = "AE 83 10 06";

float targetDistance = 3.6;

// daily feeding limits
int dog1Count = 0;
int dog2Count = 0;
int dailyLimit = 2;

float getDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000); // timeout to avoid hanging
  if (duration == 0) return -1;

  float distance = duration * 0.034 / 2.0;
  return distance;
}

int motorSpeed = 80;  // about half speed

void stopMotor() {
  analogWrite(MOTOR_PIN, 0);
}

void startMotor() {
  analogWrite(MOTOR_PIN, motorSpeed);
}

void dispenseUntilFull() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Dispensing...");

  startMotor();

  while (true) {
    float distance = getDistance();

    lcd.setCursor(0, 1);
    lcd.print("Dist: ");
    if (distance < 0) {
      lcd.print("Err   ");
    } else {
      lcd.print(distance, 1);
      lcd.print(" cm ");
    }

    Serial.print("Distance: ");
    Serial.println(distance);

    if (distance > 0 && distance <= targetDistance) {
      stopMotor();
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Target Reached");
      lcd.setCursor(0, 1);
      lcd.print("Motor Stopped");
      delay(1500);
      break;
    }

    delay(200);
  }
}

void setup() {
  Serial.begin(9600);
  SPI.begin();
  myRFID.PCD_Init();

  pinMode(MOTOR_PIN, OUTPUT);
  stopMotor();

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  lcd.init();
  lcd.backlight();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Scan RFID...");
  lcd.setCursor(0, 1);
  lcd.print("Ready");

  Serial.println("Scan RFID...");
}

void loop() {
  if (!myRFID.PICC_IsNewCardPresent()) return;
  if (!myRFID.PICC_ReadCardSerial()) return;

  String content = "";
  for (byte i = 0; i < myRFID.uid.size; i++) {
    if (myRFID.uid.uidByte[i] < 0x10) content += "0";
    content += String(myRFID.uid.uidByte[i], HEX);
    if (i < myRFID.uid.size - 1) content += " ";
  }
  content.toUpperCase();

  Serial.print("UID: ");
  Serial.println(content);

  if (content == dog1UID) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Dog 1 Detected");

    if (dog1Count < dailyLimit) {
      lcd.setCursor(0, 1);
      lcd.print("Feeding...");
      dispenseUntilFull();
      dog1Count++;

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Dog 1 Meals:");
      lcd.setCursor(0, 1);
      lcd.print(dog1Count);
      delay(1500);
    } else {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Dog 1 Limit");
      lcd.setCursor(0, 1);
      lcd.print("Reached");
      Serial.println("Dog 1 limit reached!");
      delay(1500);
    }
  }
  else if (content == dog2UID) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Dog 2 Detected");

    if (dog2Count < dailyLimit) {
      lcd.setCursor(0, 1);
      lcd.print("Feeding...");
      dispenseUntilFull();
      dog2Count++;

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Dog 2 Meals:");
      lcd.setCursor(0, 1);
      lcd.print(dog2Count);
      delay(1500);
    } else {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Dog 2 Limit");
      lcd.setCursor(0, 1);
      lcd.print("Reached");
      Serial.println("Dog 2 limit reached!");
      delay(1500);
    }
  }
  else {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Access Denied");
    lcd.setCursor(0, 1);
    lcd.print("Unknown Tag");
    Serial.println("Access Denied!");
    delay(1500);
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Scan RFID...");
  lcd.setCursor(0, 1);
  lcd.print("Ready");

  Serial.println();
  delay(1000);

  myRFID.PICC_HaltA();
  myRFID.PCD_StopCrypto1();
}