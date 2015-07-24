#include <Servo.h> 
Servo leftMotor, rightMotor, clawMotor;


#define trigPin 2
#define echoPin 3
#define leftPin 10
#define rightPin 9
#define clawPin 8


int distance = -1;
int trackedObjectDistance = -1;
int lastRealDistance = -1;
double spiralLength = 2500;
boolean verifiedObjectSize = false;


const int LEFT = 1;
const int RIGHT = 0;
const int FORWARD = 5;
const int BACK = 25;
const int STOP = 42;

const int STOP_ON_DISCOVERY = 234230;
const int MOVE_TO_OBJECT = 23434;
const int STOP_ON_LOSS = 2233333;
const int JUST_GO = 9988;


const int GRASP_RANGE = 5;
const int GRASP_WIDTH = 7;
const int FULL_ROTATION_TIME = 5000;
const int TIME_PER_CM = 200;
const double RADIAN_OVERSHOOT = 15/360.0 * 2 * PI;

const int LOST_OBJECT = 883;
const int TIMEOUT = 430;
const int TOO_BIG = 98098;
const int SUCCESS = 56345;

void setup() {
  Serial.begin (9600);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  clawMotor.attach(clawPin);
  leftMotor.attach(leftPin);
  rightMotor.attach(rightPin);
  getDistanceCm();
  delay(1000);
  
  
}

void loop() {
  search();
}

void getDistanceCm() {
  double duration;
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  distance = (int)((duration/2) / 29.1);
  if (distance >= 200 || distance <= 0){
    distance = -1;
  }
  else {
    lastRealDistance = distance;
  }
  
  
}

int go(int theDirection, int timeout, int mode) {
  long startTime = millis();
  boolean discovered = false;
  if (theDirection == FORWARD) {
    rightMotor.write(0);
    leftMotor.write(180);
  }
  else if (theDirection == BACK) {
    rightMotor.write(180);
    leftMotor.write(0);
  }
  else if (theDirection == RIGHT) {
    rightMotor.write(95);
    leftMotor.write(95);
  }
  else if (theDirection == LEFT) {
    rightMotor.write(85);
    leftMotor.write(85);
  }
  else if (theDirection == STOP) {
    rightMotor.write(90);
    leftMotor.write(90);
  }
  while (millis() - startTime < timeout) {
    int previousDistance = distance;
    getDistanceCm();
    Serial.print("distance: ");
    Serial.println(distance);
    if (mode == STOP_ON_DISCOVERY) {
      if (distance != -1) {
        if (trackedObjectDistance != -1) {
          if (distance - trackedObjectDistance > 5) {
            continue;
          }
        }
        Serial.println("found object");
        
        trackedObjectDistance = distance;
        Serial.println("confirmed object");
        spiralLength = 4000;
        pointTowardsObject();
        Serial.println("moving to object");
        int result = go(FORWARD, 999999, MOVE_TO_OBJECT);
        if (result == LOST_OBJECT) {
          Serial.println("lost object, trying to relocate");
          result = findObjectAgain();
          if (result == LOST_OBJECT || result == TOO_BIG) {
            return result;
          }
        }
        else if (result == SUCCESS) {
          pointTowardsObject();
          snapClaw();
          /*int size = checkObjectSize();
          if (size == TOO_BIG) {
            return TOO_BIG;
          }
          else {
            pointTowardsObject();
            return SUCCESS;  
          }*/
          return SUCCESS;
        }
         
      
      }
    }
      

    else if (mode == MOVE_TO_OBJECT) {
      if (distance == -1 || distance - previousDistance > 5) {
        delay(100);
        getDistanceCm();
        if (distance == -1 || distance - previousDistance > 5) {
          return LOST_OBJECT;
        }
      }
      else if (distance > 0 && distance < GRASP_RANGE + 3) {
        return SUCCESS;
      }  
    }
 
    else if (mode == STOP_ON_LOSS) {
      if (!discovered) {
        if (distance != -1) {
          discovered = true;
        }
      }
      else {
        if (distance < 0 || distance - previousDistance > 5) {
          return LOST_OBJECT;
        }

      }
    }
    
    
      
  
  }
  return TIMEOUT;

}


void search() {
  Serial.println("initializing search");
  findClosest();
 
  int result = go(STOP, 100, STOP_ON_DISCOVERY);
  if (result == TIMEOUT) {
    result = findObjectAgain();
    if (result == SUCCESS) {
      result = go(STOP, 100, STOP_ON_DISCOVERY);
    }
  }
  trackedObjectDistance = -1;
  verifiedObjectSize = false;
  go(RIGHT, FULL_ROTATION_TIME/3, JUST_GO);
  go(FORWARD, spiralLength, JUST_GO);
  //spiralLength += 1000;
  

}

int pointTowardsObject() {
  Serial.println("pointing towards object");
  int result;
  Serial.println("going right 1");
  result = go(RIGHT, 2*FULL_ROTATION_TIME, STOP_ON_LOSS);
  if (result == LOST_OBJECT) {
    int startTime = millis();
    Serial.println("going left 1");
    result = go(LEFT, 2*FULL_ROTATION_TIME, STOP_ON_LOSS);
    if (result == LOST_OBJECT) {
      int timeToSpanObject = millis() - startTime;
      //double radians = (double)timeToSpanObject/FULL_ROTATION_TIME * 2 * PI;
      //double width = lastRealDistance * sin(radians/2 - RADIAN_OVERSHOOT) * 2;
      //Serial.print("width: ");
      //Serial.println((int)width);

      Serial.println("going right again");
      go(RIGHT, timeToSpanObject/2, JUST_GO);

      go(STOP, 0, JUST_GO);
      return SUCCESS;
    }
  }
}

/*int checkObjectSize() {
  if (verifiedObjectSize) {
    return SUCCESS;
  }
  getDistanceCm();
  int objectDistance = distance;
  go(LEFT, FULL_ROTATION_TIME/4, JUST_GO);
  go(FORWARD, TIME_PER_CM * GRASP_WIDTH/2, JUST_GO);
  go(RIGHT, FULL_ROTATION_TIME/4, JUST_GO);
  go(STOP, 500, JUST_GO);
  getDistanceCm();

  if (distance != -1 && abs(distance - objectDistance) < 5) {
    return TOO_BIG;
  }
  go(RIGHT, 0, JUST_GO);
  while (true) {
    getDistanceCm();
    if (distance != -1 && abs(distance - objectDistance) < 5) {
      go(STOP, 0, JUST_GO);
      break;
    }
  }
  verifiedObjectSize = true;
  return SUCCESS;
  
}*/


void graspObject() {
  Serial.println("grasping");
  go(STOP, 5000, JUST_GO);
}

void findClosest() {
  go(RIGHT, 0, JUST_GO); 
  int startTime = millis();
  int minDistanceTime = -1;
  int minDistance = 999999;
  while (millis() - startTime < 1.5 * FULL_ROTATION_TIME) {
    getDistanceCm();
    Serial.print("finding closest: ");
    Serial.println(distance);
    Serial.println(minDistance);
    if (distance != -1 && distance < minDistance) {
      minDistanceTime = millis();
      minDistance = distance;
    }
  }
  int endTime = millis();
  trackedObjectDistance = minDistance;
  go(RIGHT, 2*FULL_ROTATION_TIME, STOP_ON_DISCOVERY);    

}

int findObjectAgain() {
  int result = go(RIGHT, FULL_ROTATION_TIME/8, STOP_ON_DISCOVERY);
  if (result == TIMEOUT) {
    go(LEFT, FULL_ROTATION_TIME/4, STOP_ON_DISCOVERY);
    if (result == TIMEOUT) {
      return LOST_OBJECT;
      Serial.println("lost object forever");
    }
    else if (result == TOO_BIG) {
      return TOO_BIG;
    }
  } 
 return SUCCESS; 
}

void snapClaw() {

  clawMotor.write(180);
  delay(300);
  clawMotor.write(0);
  delay(300);
  clawMotor.write(180);
  delay(300);
  clawMotor.write(0);
  delay(300);
  clawMotor.write(180);
  delay(300);
  clawMotor.write(0);
  delay(300);
  
}
    
    
  
      
     
