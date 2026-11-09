#include <QTRSensors.h>
#include <SD.h>

/*plan
  dance!

  otherwise:
  find the line
  move along it, when you see that you are off to one side, stop and turn slightly
  first check how fast you are moving
  and schedule a turn back when you think you'll be centered
  if you see a color
  write it down
  next time the line sensors do something funky
  turn 90 degrees or as close as you can

  once you see green, ...? do a spin looking for timmy
  then... write down where he is and hope
  if you miss, go back to spinning
*/

// #define ultra 2
// #define motor1 3
// #define motor2 4
// #define motorspeed 5
// #define motor4 6
// #define line3 7
// #define line4 8
// #define line5 9 //right from front
// #define color1 A0 //s3
// #define color2 A1 //s2
// #define color3 A2 //output
// #define line2 A3
// #define line1 A4 //left from front
// #define motor3 A5
#define ultra 2
#define motor1 3
#define motor2 4
#define motor3 A5
#define motor4 6
#define motorspeed 5
// #define line1 A4 //left from front
#define line1 A3
#define line2 7
#define line3 8
#define line4 9 //right from front
#define color1 A0 //s3
#define color2 A1 //s2
#define color3 A2 //output
#define CSpin 10

bool dancing = false;

int timeForTurn = 5;
int turnSpeed = 130;
int moveSpeed = 130;
int timmySpeed = 1023;
float millisecondsPerUnitPerMillisecond = 0.001f;

unsigned long endInwardsTurn = 0;
bool fixTurnIsLeft = true;
int fixTurnDuration = 0;
unsigned long fixTurnSchedule = 0;
float unitsPerMillisecondDriftRight = 0;
bool lastSeenColorWasLeft = true;
unsigned long lastSeenColor = 0;
int lookingForTimmy = 0;
unsigned long stopSpinningNow = 0;
int nearestTimmySeen = 10^9;
bool lookingAtTimmy = false;
unsigned long weMissed = 0;
int pushTime = 5000;

int ultraDist;
int R;
int G;
int B;

QTRSensors qtr;
const uint8_t SensorCount = 4;
uint16_t sensorValues[SensorCount];
// uint16_t lastValues[SensorCount];

File SDOutput;
unsigned long danceStart;
String dance;
char moves[64];
int danceStep;

int lineBias;
int lastLineBias;
float lineBiasSpeed;
const float lineBiasSpeedPreservation = 0.95f;
bool returning = false;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(ultra, OUTPUT); // this gets switched back and forth later
  pinMode(motor1, OUTPUT);
  pinMode(motor2, OUTPUT);
  pinMode(motor3, OUTPUT);
  pinMode(motor4, OUTPUT);
  pinMode(motorspeed, OUTPUT);
  pinMode(line1, INPUT);
  pinMode(line2, INPUT);
  pinMode(line3, INPUT);
  pinMode(line4, INPUT);
  // pinMode(line5, INPUT);
  pinMode(color1, OUTPUT);
  pinMode(color2, OUTPUT);
  pinMode(color3, INPUT);
  pinMode(CSpin, OUTPUT);
  qtr.setTypeRC();
  qtr.setSensorPins((const uint8_t[]){line1, line2, line3, line4}, SensorCount);
  for (int i = 0; i < 200; i++) {
    qtr.calibrate();
  }
  Serial.println("calibrated");

  dancing = true;
  if (!SD.begin(CSpin)) {
    Serial.println("initialization failed!");
    dancing = false;
    // return;
  }
  Serial.println("initialization done.");

  SDOutput = SD.open("dance.txt");

  if (SDOutput) {
    Serial.print("dance.txt:");
    if (SDOutput.available()) {
      dance = SDOutput.read();
      Serial.println(dance);
    }
    // close the file:
    SDOutput.close();
    Serial.println("done.");
    dance.toCharArray(moves, dance.length());
  } else {
    Serial.println("error opening dance.txt");
    dancing = false;
  }
  danceStart = millis();

  // dance = "FFLLBBRR";
  // dance.toCharArray(moves, dance.length());
  // dancing = true;
}

void loop() {
  // analogWrite(motorspeed, moveSpeed);
  // digitalWrite(motor1, HIGH);
  // digitalWrite(motor2, LOW);
  // digitalWrite(motor3, LOW);
  // digitalWrite(motor4, HIGH);
  // return;
  // put your main code here, to run repeatedly:
  // Serial.print(lookingForTimmy);
  // Serial.print(" ");
  if (dancing) {
    danceStep = int(floor((millis() - danceStart)/200))%(2*dance.length());
    if (danceStep%2 == 1) {
      digitalWrite(motor1, LOW);
      digitalWrite(motor2, LOW);
      digitalWrite(motor3, LOW);
      digitalWrite(motor4, LOW);
      return;
    }
    danceStep = danceStep/2;
    Serial.println(moves[danceStep]);
    switch (moves[danceStep]) {
      case 'L':
        analogWrite(motorspeed, timmySpeed);
        digitalWrite(motor1, LOW);
        digitalWrite(motor2, HIGH);
        digitalWrite(motor3, LOW);
        digitalWrite(motor4, HIGH);
        break;
      case 'R':
        analogWrite(motorspeed, timmySpeed);
        digitalWrite(motor1, HIGH);
        digitalWrite(motor2, LOW);
        digitalWrite(motor3, HIGH);
        digitalWrite(motor4, LOW);
        break;
      case 'F':
        analogWrite(motorspeed, timmySpeed);
        digitalWrite(motor1, HIGH);
        digitalWrite(motor2, LOW);
        digitalWrite(motor3, LOW);
        digitalWrite(motor4, HIGH);
        break;
      case 'B':
        analogWrite(motorspeed, timmySpeed);
        digitalWrite(motor1, LOW);
        digitalWrite(motor2, HIGH);
        digitalWrite(motor3, HIGH);
        digitalWrite(motor4, LOW);
        break;
    }
  } else {
    if (lookingForTimmy > 0) {
      if (ultraDist > 0) {
        lookingForTimmy += 4;
        analogWrite(motorspeed, timmySpeed);
        digitalWrite(motor1, HIGH);
        digitalWrite(motor2, LOW);
        digitalWrite(motor3, LOW);
        digitalWrite(motor4, HIGH);
      } else {
        lookingForTimmy *= 0.999f;
        lookingForTimmy -= 1;
        analogWrite(motorspeed, turnSpeed);
        digitalWrite(motor1, LOW);
        digitalWrite(motor2, HIGH);
        digitalWrite(motor3, LOW);
        digitalWrite(motor4, HIGH);
      }
    } else {
      if (ultraDist > 0) {
        lookingForTimmy = 1;
      }
      //determine if we're off the line
      // lineBias = sensorValues[0] + sensorValues[1] - sensorValues[2] - sensorValues[3];
      if (false && millis()-lastSeenColor < 100) {
        //turn 90 degrees
        //we don't need you just follow the line
      } else if (abs(lineBias-1500) > 500) {
        analogWrite(motorspeed, turnSpeed);
        digitalWrite(motor1, ((lineBias-1500)>0?LOW:HIGH));
        digitalWrite(motor2, ((lineBias-1500)>0?HIGH:LOW));
        digitalWrite(motor3, ((lineBias-1500)>0?LOW:HIGH));
        digitalWrite(motor4, ((lineBias-1500)>0?HIGH:LOW));
      } else {
        analogWrite(motorspeed, moveSpeed);
        digitalWrite(motor1, HIGH);
        digitalWrite(motor2, LOW);
        digitalWrite(motor3, LOW);
        digitalWrite(motor4, HIGH);
      }
      /*
      } else if (millis() < endInwardsTurn) {
        //turning in, no action
      } else if (abs(lineBias-1500) > 500 && !returning) {
        endInwardsTurn = millis() + millisecondsPerUnitPerMillisecond * unitsPerMillisecondDriftRight * 2;
        analogWrite(motorspeed, moveSpeed);
        digitalWrite(motor1, (lineBias>0?LOW:HIGH));
        digitalWrite(motor2, (lineBias>0?HIGH:LOW));
        digitalWrite(motor3, (lineBias>0?HIGH:LOW));
        digitalWrite(motor4, (lineBias>0?LOW:HIGH));
        fixTurnIsLeft = lineBias<=0;//?
        fixTurnDuration = millisecondsPerUnitPerMillisecond * unitsPerMillisecondDriftRight;
        fixTurnSchedule = endInwardsTurn + 1*abs(unitsPerMillisecondDriftRight);
        returning = true;
      } else if (millis() < fixTurnSchedule) {
        //moving back
        analogWrite(motorspeed, moveSpeed);
        digitalWrite(motor1, HIGH);
        digitalWrite(motor2, LOW);
        digitalWrite(motor3, LOW);
        digitalWrite(motor4, HIGH);
      } else if (millis() < fixTurnSchedule + fixTurnDuration) {
        analogWrite(motorspeed, moveSpeed);
        digitalWrite(motor1, (fixTurnIsLeft>0?LOW:HIGH));
        digitalWrite(motor2, (fixTurnIsLeft>0?HIGH:LOW));
        digitalWrite(motor3, (fixTurnIsLeft>0?HIGH:LOW));
        digitalWrite(motor4, (fixTurnIsLeft>0?LOW:HIGH));
      } else {
        analogWrite(motorspeed, moveSpeed);
        digitalWrite(motor1, HIGH);
        digitalWrite(motor2, LOW);
        digitalWrite(motor3, LOW);
        digitalWrite(motor4, HIGH);
      }
      */
    }
    // Serial.print("processing ");
    // Serial.print(millis()-time);
    // time = millis();
    readUltrasonic();
    // Serial.print("ultra ");
    // Serial.print(millis()-time);
    // time = millis();
    // readLines();
    // readColor();
    // Serial.print("color ");
    // Serial.print(millis()-time);
    // time = millis();
    // driveMotors();
  }

  //update the speed
  lineBias = qtr.readLineBlack(sensorValues);
  lineBiasSpeed *= lineBiasSpeedPreservation;
  lineBiasSpeed += (lineBias-lastLineBias)*(1-lineBiasSpeedPreservation);
  lastLineBias = lineBias;
}

void readUltrasonic() {
  pinMode(ultra, OUTPUT);

  digitalWrite(ultra, LOW);
  delayMicroseconds(2);
  digitalWrite(ultra, HIGH);
  delayMicroseconds(5);
  digitalWrite(ultra, LOW);

  pinMode(ultra, INPUT);

  ultraDist = pulseIn(ultra, HIGH, 4000);
  // Serial.println(ultraDist);
}

void readColor() {
  digitalWrite(color2, LOW);
  digitalWrite(color1, LOW);
  R = pulseIn(color3, LOW, 2000);

  digitalWrite(color2, HIGH);
  digitalWrite(color1, HIGH);
  G = pulseIn(color3, LOW, 2000);

  digitalWrite(color2, LOW);
  digitalWrite(color1, HIGH);
  B = pulseIn(color3, LOW, 2000);

  // Serial.print(String(R)+" "+String(G)+" "+String(B));
}
