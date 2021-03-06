#include <Servo.h>
#include <EEPROM.h>

#define RELAY           5 // D1
#define BUTTON          4 // D2
#define SERVO           0 // D3
#define BLUE_LED        2 // D4

#define PUSHED          LOW
#define ENDPOS          20
#define SERVO_STANDBY   160

#define FORWARD         1
#define REVERSE         2

#define CMD_READ        1
#define CMD_WRITE       2
#define CMD_MODE        3
#define CMD_SERVO       4
#define CMD_EE_READ     5

#define SCENARIO_INIT        -1
#define SCENARIO_0            0
#define SCENARIO_1            1
#define SCENARIO_2            2
#define SCENARIO_3            3
#define SCENARIO_4            4
#define MAX_SCENARIO          4

#define STATE_STANDBY         0
#define STATE_SELECT_SCENARIO 1
#define STATE_RUN_SCENARIO    2

#define TIMEOUT         15000

#define MAX_INDEX       45
struct { 
  unsigned int val = 0;
} eepromdata;

Servo myservo;  // create servo object to control a servo
int button = 0;
int btnLock = 0;
int state = STATE_STANDBY;
int scenarioStep = 0;
int pos = SERVO_STANDBY;
int scenario = SCENARIO_INIT;
int addr = 0; // only 1 address used to store the random list index
unsigned int eepromindex = 0;
int randomValues[] = {0,4,2,1,3,1,2,4,3,1,3,2,0,1,3,0,2,1,4,1,2,1,0,2,4,4,1,1,3,1,0,3,4,2,2,4,1,3,2,4,1,2,3,2,1};

unsigned long timeout = millis() + TIMEOUT;
int stateMain = STATE_STANDBY;
int stateScenario = STATE_STANDBY;
unsigned long countdown = 0;
int inDiagnosis = 0;

void setup() {
  Serial.begin(115200);
  //enablePhaseLockedWaveform();
  EEPROM.begin(4);  //Initialize EEPROM

  Serial.print("EEPROM index: ");
  EEPROM.get(addr, eepromdata);
  Serial.println(eepromdata.val);
  eepromindex = eepromdata.val;

  // flush serial line
  while(Serial.available() > 0) {
    char t = Serial.read();
  }
  myservo.attach(SERVO, 544, 2400);  // attaches the servo (redefine ranges s they were changed in ESP8266 V3.0.0)
  myservo.write(SERVO_STANDBY); // start servo at STANDBY deg
  delay(200);                   // wait for servo to be in position
  pinMode(BUTTON, INPUT);       // button pin as input (no pull-up)
  pinMode(BLUE_LED, OUTPUT);    // led pin as output
  pinMode(RELAY, OUTPUT);       // relay pin as output
  digitalWrite(RELAY, 1);       // set the relay as a latch
  Serial.println("Setup completed");
  delay(50);
  if(!isPushed()){
    inDiagnosis = 1;
    Serial.println("In diagnosis.");
  }
}

void scenarioStepMoveToContact(int delayscenarioStep, int speed){
  pos -= speed;
  myservo.write(pos);                 // tell servo to go to position in variable 'pos'
  delay(delayscenarioStep);
  if(!isPushed()){
    Serial.println("Reached contact");
    scenarioStep++;
    btnLock = 0; // unlock next cycle
    Serial.println("unlock");
  }
  /* Safety */
  else if(pos<ENDPOS){
    Serial.println("Reached end position");
    btnLock = 0; // unlock next cycle [DEBUG]
    Serial.println("Unlock");
    scenarioStep++;
  }
}

void scenarioStepWaitForContact(){
  if(isPushed()){
    Serial.println("Button pushed");
    scenarioStep++;
  }
  else if(millis() > countdown){
    scenarioStep += 2;
    Serial.println("Timed out");
  }
}

void scenarioStepWait(){
  if(millis() > countdown){
    scenarioStep++;
    Serial.println("Delay over");
  }
}

void scenarioStepMoveToPosition(int target, int delayscenarioStep, int direction, int speed){
  if(target>pos) pos += speed;
  else pos -= speed;
  myservo.write(pos);                 // tell servo to go to position in variable 'pos'
  delay(delayscenarioStep);           // waits 5ms for the servo to reach the position
  /* Safety */
  if(pos <= ENDPOS || pos >= SERVO_STANDBY){
    Serial.println("Extreme position");
    scenarioStep++;
  }
  else if( ( (pos >= target) && (direction == FORWARD) )
  || ( (pos <= target) && (direction == REVERSE) ) ){
    Serial.println("Target position");
    scenarioStep++;
  }
}

void scenarioStepMoveBackStandby(int delayscenarioStep, int speed){
  pos += speed;
  myservo.write(pos);                      // tell servo to go to position in variable 'pos'
  delay(delayscenarioStep);                        // waits for the servo to reach the position
  if(pos>=SERVO_STANDBY){
    Serial.println("Standby position");
    scenarioStep++;
  }
}

int isPushed(){
  int res = 0;
  int button = digitalRead(BUTTON);
  if(button == PUSHED){
    res = 1;
  }
  return res;
}

void stateStandby(){
  if(millis() > timeout){
    Serial.println("Exit");
    exit();
    inDiagnosis = 1;
    //timeout = millis()+12000; // only for debug
  }
  else if(isPushed() && !btnLock){
    stateMain = STATE_SELECT_SCENARIO;
    btnLock = 1;
    Serial.println("Lock");
    delay(5);
    Serial.println("Button contact. Go to scenario selection.");
    delay(5);
    timeout = millis() + TIMEOUT;
  }
  else delay(5);
}

void selectScenario(){
  //if(scenario >= MAX_SCENARIO) scenario = 0;
  //else scenario++;

  eepromindex ++;
  if(eepromindex >= MAX_INDEX) eepromindex = 0;
  scenario = randomValues[eepromindex];

  Serial.print("Current index: ");
  Serial.println(eepromindex);
  
//scenario = 0;
  Serial.print("Current scenario: ");
  Serial.println(scenario);
  stateMain = STATE_RUN_SCENARIO;
  scenarioStep = 0;
}

void runScenario(){
  if(scenario == SCENARIO_0) scenario_01(0, 8);
  else if(scenario == SCENARIO_1) scenario_01(0, 4);
  else if(scenario == SCENARIO_2) scenario_02(0, 8);
  else if(scenario == SCENARIO_3) scenario_03(0, 8);
  else if(scenario == SCENARIO_4) scenario_04(0, 6);
  else{
    stateMain = STATE_STANDBY;
    scenario = SCENARIO_0;
  }
}

void exit(){
  eepromdata.val = eepromindex;
  EEPROM.put(addr, eepromdata);
  EEPROM.commit();    //Store data to EEPROM
  delay(100);
  digitalWrite(RELAY, 0);       // release the relay as a latch
}

void delayCountdown(){
  if(millis() > countdown) scenarioStep++;
}

/* Scenarios */

void scenario_01(int speedDelay, int speed){
  switch(scenarioStep){
    case 0:
    default:
      scenarioStepMoveToContact(speedDelay, speed);
      break;
    case 1:
      scenarioStepMoveBackStandby(speedDelay, speed);
      break;
    case 2:
      stateMain = STATE_STANDBY;
      scenarioStep = 0;
      break;
  }
}

void scenario_02(int speedDelay, int speed){
  switch(scenarioStep){
    case 0:
    default:
      scenarioStepMoveToPosition(140, 15, FORWARD, speed);
      countdown = millis()+1500; // for next scenarioStep
      break;
    case 1:
      delayCountdown();
      break;
    case 2:
      scenarioStepMoveToContact(speedDelay, speed);
      break;
    case 3:
      scenarioStepMoveBackStandby(speedDelay, speed);
      break;
    case 4:
      stateMain = STATE_STANDBY;
      scenarioStep = 0;
      break;
  }
}

void scenario_03(int speedDelay, int speed){
  //Serial.print("Step: ");Serial.println(scenarioStep);
  switch(scenarioStep){
    case 0:
    default:
      scenarioStepMoveToContact(speedDelay, speed);
      break;
    case 1:
      scenarioStepMoveToPosition(140, speedDelay, FORWARD, speed);
      countdown = millis()+5000; // for next scenarioStep
      break;
    case 2:
      scenarioStepWaitForContact(); // scenarioStep +=2 if timeout
      break;
    case 3:
      scenarioStepMoveToContact(speedDelay, speed);
      break;
    case 4:
      scenarioStepMoveBackStandby(speedDelay, speed);
      break;
    case 5:
      stateMain = STATE_STANDBY;
      scenarioStep = 0;
      break;
  }
}

void scenario_04(int speedDelay, int speed){
  //Serial.print("Step: ");Serial.println(scenarioStep);
  switch(scenarioStep){
    case 0:
    default:
      scenarioStepMoveToPosition(140, speedDelay, FORWARD, speed);
    case 1:
      scenarioStepMoveToPosition(120, speedDelay, REVERSE, speed);
      break;
    case 2:
      scenarioStepMoveToPosition(140, speedDelay, FORWARD, speed);
      break;
    case 3:
      scenarioStepMoveToPosition(120, speedDelay, REVERSE, speed);
      break;
    case 4:
      scenarioStepMoveToPosition(140, speedDelay, FORWARD, speed);
      break;
    case 5:
      scenarioStepMoveToPosition(120, speedDelay, REVERSE, speed);
      break;
    case 6:
      scenarioStepMoveToPosition(140, speedDelay, FORWARD, speed);
      break;
    case 7:
      scenarioStepMoveToPosition(120, speedDelay, REVERSE, speed);
      countdown = millis()+2000; // for next scenarioStep
      break;
    case 8:
      scenarioStepWait(); // scenarioStep +=2 if timeout
      break;
    case 9:
      scenarioStepMoveToContact(speedDelay, speed);
      break;
    case 10:
      scenarioStepMoveBackStandby(0, speed);
      break;
    case 11:
      stateMain = STATE_STANDBY;
      scenarioStep = 0;
      break;
  }
}

/* Test useless box components */
void test(){
  // if there's any serial available, read it:
  while (Serial.available() > 0) {
    
    // look for the next valid integer in the incoming serial stream:
    int command = Serial.parseInt();
    // do it again:
    int pin = Serial.parseInt();
    // do it again:
    int parameter = Serial.parseInt();

    // look for the newline. That's the end of your sentence:
    if (Serial.read() == '\r') {
      Serial.print("Rx: command: "); Serial.print(command);
      Serial.print(", pin: ");Serial.print(pin);
      Serial.print(", parameter: "); Serial.println(parameter);
      delay(100);
      // block into diag mode for cycle
      inDiagnosis = 1;
      if(command == CMD_WRITE){
        Serial.print("WRITE: D");
        Serial.print(pin);
        Serial.print(" = ");
        Serial.println(parameter);
        digitalWrite(pin, parameter);
      }
      else if(command == CMD_READ){
        Serial.print("READ: D");
        Serial.print(pin);
        Serial.print(" = ");
        Serial.println(digitalRead(pin));
      }
      else if(command == CMD_MODE){
        Serial.print("MODE: D");
        Serial.print(pin);
        Serial.print(" = ");
        Serial.println(parameter);
        //pinMode(pin, parameter);
        pinMode(pin, OUTPUT);
      }
      else if(command == CMD_SERVO){
        Serial.print("SERVO: ");
        Serial.println(parameter);
        myservo.write(parameter);
      }
      else if(command == CMD_EE_READ){
        Serial.print("EEPROM index: ");
        EEPROM.get(addr, eepromdata);
        Serial.println(eepromdata.val);
      }
    }
  }
}

void loop() {
  /* Check if serial data available and go in diagnosis mode */
  test();

  if(!inDiagnosis){
    switch(stateMain){
      case STATE_STANDBY:
      default:
        stateStandby();
        break;
      case STATE_SELECT_SCENARIO:
        selectScenario();
        break;
      case STATE_RUN_SCENARIO:
        runScenario();
        break;
    }
  }
}
