/*
  Behavior Control with Webserver Counts
 
 General template:
 Collects input from digital inputs (e.g., beam breaks over food wells),
 and controls digital outputs (e.g., syringe pumps or valves) to provide
 reward as appropriate.
 
 This file:
 Runs the logic for a linear (i.e., back and forth) maze.
 Syringe pumps are active LOW
 WEBSERVER REMOVED!!! SOMETHING WEIRD IS GOING ON!!!
 
 by Caleb Kemere and Nicola Park, 2013
 based on a weather webserver by Tom Igoe
 modified by Etienne Ackermann, 01/21/2014
 modified by Eric Lewis, Jon Towne 09/12/2014
 modified by Eric Lewis, 09/25/2014
 */

#include "TimerOne.h"
#include "CmdMessenger.h"

// Arduino input pins corresponding to food well beam breaks
const int beamBreakA = A4;
const int beamBreakB = A5;

// Arduino output pins corresponding to syringe pump output controls
const int pumpA = 6;
const int pumpB = 7;

const int dispenseLength = 1000; // duration (in ms) that pump remains active per reward

// State information for behvaioral logic
//enum wellType {none, wellA, wellB, wellC};
enum WellType {
  none,
  wellA,
  wellB,
  wellMulti };

int i;

// Camera trigger state used by timer interrupt
int triggerState = LOW;

volatile int triggerPin = A1;  // the pin with a LED was 13
volatile unsigned long int triggerCounter = 0;
volatile unsigned long int lastTriggerTime = 0;
unsigned long int trackingTriggerCounter = 0;
unsigned long int trackingTriggerTime = 0;
volatile int logCameraTriggers = 0; // flag to denote logging

int lastWell;
int activatedWell;
int fakeActivatedWell = none;

// State information for output control

unsigned long int dispenseTime = 0; // will be the time dispensing should end

// Reward history for webserver output
int wellACount = 0;
int wellBCount = 0;
unsigned long time;


// Attach a new CmdMessenger object to the default Serial port
CmdMessenger cmdMessenger = CmdMessenger(Serial);

// This is the list of recognized commands. These can be commands that can either be sent or received. 
// In order to receive, attach a callback function to these events
enum
{
  kStatus,           // Generic command for status messages
  kEvent,            // Command for well events
  kClearWellLog,     // Command to clear well counters
  kSetFramePeriod,   // Command to set camera triggering frame rate (argument: period in microseconds)
  kSetTriggerPin,    // Command to set camera trigger pin (argument: pin number)
  kEnableTrigger,    // Command to start/stop triggering (argument: true=on, false=off)
  kEnableTriggerLogging, // Command to enable trigger *logging*
  kSetEventPin,      // Command to set IO event pin (argument: pin number)
  kFakeBeamBreak,    // Command to fake a well visit (argument: character A B or C)
  kVersion,          // Command to ask for version info
};

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
}

// Called when a received command has no attached function
void OnUnknownCommand()
{
  cmdMessenger.sendCmd(kStatus,"Command without attached callback");
}

// Report version
void OnVersion()
{
  cmdMessenger.sendCmd(kVersion,"Linear-Maze RewardControl v1.0");
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
  cmdMessenger.sendCmdStart(kStatus);
  cmdMessenger.sendCmdArg("Clearing well counters. Current counts");
  cmdMessenger.sendCmdArg((int) wellACount);  
  cmdMessenger.sendCmdArg((int) wellBCount);  
  cmdMessenger.sendCmdArg((int) wellACount + wellBCount);  
  cmdMessenger.sendCmdEnd();
  wellACount = 0; wellBCount = 0;
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


void setup() {

  // initalize the input and output pins:
  pinMode(beamBreakA, INPUT);
  pinMode(beamBreakB, INPUT);
  pinMode(pumpA, OUTPUT);
  pinMode(pumpB, OUTPUT);
  
  pinMode(triggerPin, OUTPUT);

  digitalWrite(pumpA, HIGH); // initially turn syringe pumps off
  digitalWrite(pumpB, HIGH); //   (they're active LOW)

  Timer1.initialize(16666); // 30 frames per second
  Timer1.attachInterrupt(triggerCamera); // triggerCamera to run every 0.15 seconds

  // Listen on serial connection for messages from the PC
  Serial.begin(115200); 
  // Adds newline to every command
  cmdMessenger.printLfCr();   

  // Attach my application's user-defined callback methods
  attachCommandCallbacks();

  // Send the status to the PC that says the Arduino has booted
  // Note that this is a good debug function: it will let you also know 
  // if your program had a bug and the arduino restarted  
  OnVersion();
  
  lastWell = none;
  
}

void loop() {   
  time=millis();
  
  if (logCameraTriggers) {
    if (triggerCounter > trackingTriggerCounter) {
      trackingTriggerTime = lastTriggerTime; // ideally we would mutext these two lines
      trackingTriggerCounter = triggerCounter;
      LogFrameTrigger();
    }
  }
  
  checkBeamBreaks();
  
  // =================================================================================================
  // Task logic
  
  if (activatedWell == wellMulti) {
    // something's wrong and multiple wells were activated
  } else if (activatedWell != none) {   
    if (activatedWell != lastWell) { // ignore repeated visits to a well
      LogWellVisit();
      
      RewardWell(activatedWell);

      lastWell = activatedWell;
    }
  }
  
  // =================================================================================================

  DoDispense();
  
  // Process incoming serial data, and perform callbacks
  cmdMessenger.feedinSerialData();  
}

void checkBeamBreaks(void) {
  int aflag, bflag, newWell;
  
  aflag = digitalRead(beamBreakA); // HIGH = 1, LOW = 0!
  bflag = digitalRead(beamBreakB); // HIGH = 1, LOW = 0!
  if ((aflag + bflag) == 0)
    newWell = none;
  if ((aflag + bflag) > 1)
    activatedWell = wellMulti;    
  if (aflag)
    activatedWell = wellA;
  if (bflag)
    activatedWell = wellB;
    
  if (fakeActivatedWell != none) {
    if ( (fakeActivatedWell == wellA) ||
         (fakeActivatedWell == wellB) ) {
      activatedWell = fakeActivatedWell;
         }
    fakeActivatedWell = none;
  }
}

void LogWellVisit (void) {
  char wellChar;
  
  wellChar = 'A' - 1 + activatedWell;
  cmdMessenger.sendCmdStart(kEvent);
  cmdMessenger.sendCmdArg((unsigned long) time);
  cmdMessenger.sendCmdArg("W");
  cmdMessenger.sendCmdArg((char) wellChar);
  cmdMessenger.sendCmdEnd();
}

void LogFrameTrigger (void) {
  cmdMessenger.sendCmdStart(kEvent);
  cmdMessenger.sendCmdArg((unsigned long) trackingTriggerTime);
  cmdMessenger.sendCmdArg("T");
  cmdMessenger.sendCmdArg((unsigned long) trackingTriggerCounter);
  cmdMessenger.sendCmdEnd();
}

void RewardWell(int whichOne) {
  char wellChar;
  int wellCount;
  
  wellChar = '0';
  
  switch (whichOne) {
  case wellA:
    digitalWrite(pumpB, HIGH); digitalWrite(pumpA, LOW);
    wellChar = 'A';
    wellCount = ++wellACount;
    break;
  case wellB:
    digitalWrite(pumpA, HIGH); digitalWrite(pumpB, LOW);
    wellChar = 'B';
    wellCount = ++wellBCount;
    break;

  }
  dispenseTime = millis() + dispenseLength;
  
  cmdMessenger.sendCmdStart(kEvent);
  cmdMessenger.sendCmdArg((unsigned long) time);
  cmdMessenger.sendCmdArg("R");
  cmdMessenger.sendCmdArg((char) wellChar);
  cmdMessenger.sendCmdArg((int) wellCount);  
  cmdMessenger.sendCmdArg((int) wellACount + wellBCount);  
  cmdMessenger.sendCmdEnd();
}

void DoDispense() {
  if ((millis() > dispenseTime) && (dispenseTime != 0)) { // done dispensing
    digitalWrite(pumpA,HIGH); // (these are active low outputs)
    digitalWrite(pumpB,HIGH); // (these are active low outputs)
    dispenseTime = 0;
  }
}



 




