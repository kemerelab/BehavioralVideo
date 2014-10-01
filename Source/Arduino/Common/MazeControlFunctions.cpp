/*
  Behavior Control firmware
  Common functions used with all maze-type architectures.
 */

#include "MazeControlFunctions.h"


// Variables used to communicate which wells have been activated
int activatedWell;
unsigned long int eventTime;
int fakeActivatedWell = NONE; // used to remotely trigger logic

void CheckBeamBreaks(void) {
  int i;
  int counter;

  counter = 0;
  
  for (i = 0; i < nFoodWells; i++) {
    beamBreakFlags[i] = digitalRead(beamBreakPins[i]);
    if (beamBreakFlags[i] == HIGH) {
      counter++;
      activatedWell = i;
      eventTime = millis();
    }
  }
  if (counter == 0)
    activatedWell = NONE;
  if (counter > 1)
    activatedWell = MULTI_WELL; // error condition
    
  if (fakeActivatedWell != NONE) {
    activatedWell = fakeActivatedWell;
    fakeActivatedWell = NONE;
  }
}

// Information for output control
int dispenseLength = 1000; // duration (in ms) that pump remains active per reward
unsigned long int dispenseTime = 0; // will be the time dispensing should end


// ----------------------------------------------------------------------------
// Variables used for camera triggering

// Camera trigger state used by timer interrupt
int triggerState = LOW;
volatile int triggerPin = TRIGGER_PIN;  // the pin with a LED was 13
volatile unsigned long int triggerCounter = 0;
volatile unsigned long int lastTriggerTime = 0;
unsigned long int trackingTriggerCounter = 0;
unsigned long int trackingTriggerTime = 0;
volatile int logCameraTriggers = 0; // flag to denote logging
// ----------------------------------------------------------------------------


// Attach a new CmdMessenger object to the default Serial port
CmdMessenger cmdMessenger = CmdMessenger(Serial);

// Callbacks define on which received commands we take action
void attachCommandCallbacks()
{
  // Attach callback methods
  cmdMessenger.attach(OnUnknownCommand);
  cmdMessenger.attach(kSetFramePeriod, OnSetFramePeriod);
  cmdMessenger.attach(kSetTriggerPin, OnSetTriggerPin);
  cmdMessenger.attach(kEnableTrigger, OnEnableTrigger); // equivalent of start and stop
  cmdMessenger.attach(kEnableTriggerLogging, OnEnableTriggerLogging);
  cmdMessenger.attach(kFakeBeamBreak, OnFakeBeamBreak);  
  cmdMessenger.attach(kClearWellLog, OnClearWellLog);
  cmdMessenger.attach(kVersion, OnVersion);
  cmdMessenger.attach(kQueryPins, OnQueryPins);
  cmdMessenger.attach(kQueryWellLog, OnQueryWellLog);
}

// Called when a received command has no attached function
void OnUnknownCommand()
{
  cmdMessenger.sendCmd(kStatus,"Command without attached callback");
}

// Report version
void OnVersion()
{
  cmdMessenger.sendCmd(kVersion,"RewardControl v1.1");
  cmdMessenger.sendCmd(kStatus, maze_type); 
}

// Callback function that sets camera frame period
void OnSetFramePeriod()
{
  unsigned long int period;
  // Read period argument, interpret string as unsigned long int (Int32)
  period = cmdMessenger.readInt32Arg();
  Timer1.setPeriod(period >> 1); // divide period by two to account for alternation
  // Send back status message about period
  cmdMessenger.sendCmdStart(kStatus);
  cmdMessenger.sendCmdArg("Trigger period set");
  cmdMessenger.sendCmdArg((unsigned long int)period >> 1);
  cmdMessenger.sendCmdEnd();
}

// Callback function that sets camera frame period
void OnSetTriggerPin()
{
  unsigned int pin;
  // Read period argument, interpret string as unsigned long int (Int32)
  pin = cmdMessenger.readInt16Arg();
  triggerPin = pin;
}

// Callback function that starts or stops camera triggering
void OnEnableTrigger()
{
  unsigned int enable;
  // Read period argument, interpret string as unsigned long int (Int32)
  enable = cmdMessenger.readInt16Arg();
  if (enable == 0) {
    Timer1.stop();
    triggerState = LOW;
    digitalWrite(triggerPin, triggerState);
  }
  else {
    Timer1.restart();
  }
}

// Callback function that starts or stops logging of camera frame times
//  our assumption is that this is always called while triggering is disabled
void OnEnableTriggerLogging()
{
  unsigned int enable;
  // Read period argument, interpret string as unsigned long int (Int32)
  enable = cmdMessenger.readInt16Arg();
  if (enable == 0) {
    logCameraTriggers = 0;
  }
  else {
    triggerCounter = 0;
    logCameraTriggers = 1;
  }
}


// Callback function to test well logic
void OnFakeBeamBreak()
{
  unsigned int well;
  // Read period argument, interpret string as unsigned long int (Int32)
  well = cmdMessenger.readInt16Arg();
  fakeActivatedWell = well;
}

// Callback function to clear well counters
void OnClearWellLog()
{
  int i;
  int total;

  total = 0;
  cmdMessenger.sendCmdStart(kStatus);
  cmdMessenger.sendCmdArg("Clearing well counters. Current counts");
  for (i = 0; i < nFoodWells; i++) {
    cmdMessenger.sendCmdArg((int) wellRewardCounts[i]);  
    total = total + wellRewardCounts[i];
    wellRewardCounts[i] = 0;
  }
  cmdMessenger.sendCmdArg((int) total);  
  cmdMessenger.sendCmdEnd();
}

// Callback function to report pin mappings
void OnQueryPins()
{
  int i;
  cmdMessenger.sendCmdStart(kQueryPins);
  for (i = 0; i < nFoodWells; i++) {
    cmdMessenger.sendCmdArg((int) beamBreakPins[i]);  
    cmdMessenger.sendCmdArg((int) pumpPins[i]);  
  }
  cmdMessenger.sendCmdArg((int) triggerPin);
  cmdMessenger.sendCmdEnd();
}

// Callback function to report well counters
void OnQueryWellLog()
{
  int i;
  int total;

  total = 0;
  cmdMessenger.sendCmdStart(kQueryWellLog);
  for (i = 0; i < nFoodWells; i++) {
    cmdMessenger.sendCmdArg((int) wellRewardCounts[i]);  
    total = total + wellRewardCounts[i];
  }
  cmdMessenger.sendCmdArg((int) total);  
  cmdMessenger.sendCmdEnd();
}

void LogWellVisit (void) {
  cmdMessenger.sendCmdStart(kEvent);
  cmdMessenger.sendCmdArg((unsigned long) eventTime);
  cmdMessenger.sendCmdArg("W");
  cmdMessenger.sendCmdArg((int) activatedWell);
  cmdMessenger.sendCmdEnd();
}

void LogFrameTrigger (void) {
  if (logCameraTriggers) {
    if (triggerCounter > trackingTriggerCounter) {
      trackingTriggerTime = lastTriggerTime; // ideally we would mutext these two lines
      trackingTriggerCounter = triggerCounter;
      cmdMessenger.sendCmdStart(kEvent);
      cmdMessenger.sendCmdArg((unsigned long) trackingTriggerTime);
      cmdMessenger.sendCmdArg("T");
      cmdMessenger.sendCmdArg((unsigned long) trackingTriggerCounter);
      cmdMessenger.sendCmdEnd();
    }
  }
}

void RewardWell(int whichOne) {
  int wellCount;
  int i;
  unsigned long int eventTime;
  
  for (i = 0; i < nFoodWells; i++) {
    if (i != whichOne) {
      digitalWrite(pumpPins[i], HIGH);
      wellCount += wellRewardCounts[i];
    }
  }
  eventTime = millis();
  digitalWrite(pumpPins[whichOne], LOW);
  dispenseTime = eventTime + dispenseLength;
  wellCount += wellRewardCounts[whichOne];

  cmdMessenger.sendCmdStart(kEvent);
  cmdMessenger.sendCmdArg((unsigned long) eventTime);
  cmdMessenger.sendCmdArg("R");
  cmdMessenger.sendCmdArg((int) whichOne);
  //cmdMessenger.sendCmdArg((int) wellRewardCounts[i]);  
  //cmdMessenger.sendCmdArg((int) wellCount);  
  cmdMessenger.sendCmdEnd();
  
  OnQueryWellLog();
}

void DoDispense() {
  int i;
  
  if ((millis() > dispenseTime) && (dispenseTime != 0)) { // done dispensing
    for (i = 0; i < nFoodWells; i++) {
      digitalWrite(pumpPins[i], HIGH);
    }
    dispenseTime = 0;
  }
}

void triggerCamera(void)
{
  if (triggerState == LOW) {
    triggerState = HIGH;
    triggerCounter++;
    lastTriggerTime = millis();
  } else {
    triggerState = LOW;
  }
  digitalWrite(triggerPin, triggerState);
}

void CommonMazeSetup(void)
{
  int i;

  for (i = 0; i < nFoodWells; i++) {
    // initalize the input and output pins:
    pinMode(beamBreakPins[i], INPUT);
    pinMode(pumpPins[i], OUTPUT);
    digitalWrite(pumpPins[i], HIGH); // initially turn syringe pumps off
  }
  
  pinMode(triggerPin, OUTPUT);

  Timer1.initialize(DEFAULT_FRAMERATE); 
  Timer1.attachInterrupt(triggerCamera); // triggerCamera to run every 0.15 seconds

  // Listen on serial connection for messages from the PC
  Serial.begin(115200); 
  // Adds newline to every command
  delay(500);
  cmdMessenger.printLfCr();   

  // Attach my application's user-defined callback methods
  attachCommandCallbacks();

  // Send the status to the PC that says the Arduino has booted
  // Note that this is a good debug function: it will let you also know 
  // if your program had a bug and the arduino restarted  
  delay(500);
  OnVersion();

}

