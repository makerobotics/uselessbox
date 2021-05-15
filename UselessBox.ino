#include <Servo.h>

#define RELAY           5 // D1
#define BUTTON          4 // D2
#define SERVO           0 // D3
#define BLUE_LED        2 // D4

#define PUSHED          LOW
#define ENDPOS          55
#define SERVO_STANDBY   170

#define CMD_READ        1
#define CMD_WRITE       2
#define CMD_MODE        3
#define CMD_SERVO       4

#define MAX_SCENARIO          4

#define STATE_STANDBY         0
#define STATE_SELECT_SCENARIO 1
#define STATE_RUN_SCENARIO    2

#define TIMEOUT         15000

Servo myservo;  // create servo object to control a servo
int button = 0;
int btnLock = 0;
int state = STATE_STANDBY;
int scenarioStep = 0;
int pos = SERVO_STANDBY;
int scenario = -1;

unsigned long timeout = millis() + TIMEOUT;
int stateMain = STATE_STANDBY;
int stateScenario = STATE_STANDBY;
unsigned long countdown = 0;
int inDiagnosis = 0;

void setup() {
  Serial.begin(115200);
  // flush serial line
  while(Serial.available() > 0) {
    char t = Serial.read();
  }
  myservo.attach(SERVO);        // attaches the servo on GIOxx to the servo object
  myservo.write(SERVO_STANDBY); // start servo at STANDBY deg
  delay(500);                   // wait for servo to be in position
  pinMode(BUTTON, INPUT);       // button pin as input (no pull-up)
  pinMode(BLUE_LED, OUTPUT);    // led pin as output
  pinMode(RELAY, OUTPUT);       // relay pin as output
  digitalWrite(RELAY, 1);       // set the relay as a latch
  Serial.println("Setup completed");
  delay(50);
}

void scenarioStepMoveToContact(int delayscenarioStep){
  pos--;
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

void scenarioStepMoveToPosition(int target, int delayscenarioStep){
  if(target>pos) pos++;
  else pos--;
  myservo.write(pos);                 // tell servo to go to position in variable 'pos'
  delay(delayscenarioStep);                           // waits 5ms for the servo to reach the position
  /* Safety */
  if(pos <= ENDPOS || pos >= SERVO_STANDBY){
    Serial.println("Extreme position");
    scenarioStep++;
  }
  else if(pos == target){
    Serial.println("Target position");
    scenarioStep++;
  }
}

void scenarioStepMoveBackStandby(int delayscenarioStep){
  pos++;
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
  if(scenario >= MAX_SCENARIO) scenario = 0;
  else scenario++; 
  Serial.print("Current scenario: ");
  Serial.println(scenario);
//scenario = 3;
  stateMain = STATE_RUN_SCENARIO;
}

void runScenario(){
  if(scenario == 0) scenario_01(10);
  else if(scenario == 1) scenario_01(5);
  else if(scenario == 2) scenario_02();
  else if(scenario == 3) scenario_03();
  else if(scenario == 4) scenario_04();
  else{
    stateMain = STATE_STANDBY;
    scenario = 0;
  }
}

void exit(){
  digitalWrite(RELAY, 0);       // release the relay as a latch
}

void delayCountdown(){
  if(millis() > countdown) scenarioStep++;
}

/* Scenarios */

void scenario_01(int speedDelay){
  switch(scenarioStep){
    case 0:
    default:
      scenarioStepMoveToContact(speedDelay);
      break;
    case 1:
      scenarioStepMoveBackStandby(speedDelay);
      break;
    case 2:
      stateMain = STATE_STANDBY;
      scenarioStep = 0;
      break;
  }
}

void scenario_02(){
  switch(scenarioStep){
    case 0:
    default:
      scenarioStepMoveToPosition(150, 5);
      countdown = millis()+1500; // for next scenarioStep
      break;
    case 1:
      delayCountdown();
      break;
    case 2:
      scenarioStepMoveToContact(5);
      break;
    case 3:
      scenarioStepMoveBackStandby(15);
      break;
    case 4:
      stateMain = STATE_STANDBY;
      scenarioStep = 0;
      break;
  }
}

void scenario_03(){
  //Serial.print("Step: ");Serial.println(scenarioStep);
  switch(scenarioStep){
    case 0:
    default:
      scenarioStepMoveToContact(5);
      break;
    case 1:
      scenarioStepMoveToPosition(150, 5);
      countdown = millis()+5000; // for next scenarioStep
      break;
    case 2:
      scenarioStepWaitForContact(); // scenarioStep +=2 if timeout
      break;
    case 3:
      scenarioStepMoveToContact(5);
      break;
    case 4:
      scenarioStepMoveBackStandby(5);
      break;
    case 5:
      stateMain = STATE_STANDBY;
      scenarioStep = 0;
      break;
  }
}

void scenario_04(){
  //Serial.print("Step: ");Serial.println(scenarioStep);
  switch(scenarioStep){
    case 0:
    default:
      scenarioStepMoveToPosition(150, 5);
      break;
    case 1:
      scenarioStepMoveToPosition(120, 5);
      break;
    case 2:
      scenarioStepMoveToPosition(150, 5);
      break;
    case 3:
      scenarioStepMoveToPosition(120, 5);
      break;
    case 4:
      scenarioStepMoveToPosition(150, 5);
      break;
    case 5:
      scenarioStepMoveToPosition(120, 5);
      break;
    case 6:
      scenarioStepMoveToPosition(150, 5);
      break;
    case 7:
      scenarioStepMoveToPosition(120, 5);
      countdown = millis()+2000; // for next scenarioStep
      break;
    case 8:
      scenarioStepWait(); // scenarioStep +=2 if timeout
      break;
    case 9:
      scenarioStepMoveToContact(5);
      break;
    case 10:
      scenarioStepMoveBackStandby(2);
      break;
    case 11:
      stateMain = STATE_STANDBY;
      scenarioStep = 0;
      break;
  }
}

/* Test useless box components */
void test(){
  //delay(100);
  // if there's any serial available, read it:
  while (Serial.available() > 0) {
    
    // look for the next valid integer in the incoming serial stream:
    int command = Serial.parseInt();
    // do it again:
    int pin = Serial.parseInt();
    // do it again:
    int parameter = Serial.parseInt();

    // look for the newline. That's the end of your sentence:
    if (Serial.read() == '\n') {
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
    }
  }
}

void loop() {
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
