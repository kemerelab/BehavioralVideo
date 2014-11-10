/*
  Common functions for Behavior Control Arduino for mazes with liquid reward.
  Encompasses everything but well logic.
 */

#ifndef MAZE_CONTROL_FUNCTIONS_H
#define MAZE_CONTROL_FUNCTIONS_H

#include "TimerOne.h"
#include "CmdMessenger.h"
#include "BehaviorInterfaceCommands.h"

#ifdef TESTING
#define TRIGGER_PIN 13 // LED pin
#define LOGGING_PIN 12 
#define DEFAULT_FRAMERATE 250000
#else
#define TRIGGER_PIN A1 
#define LOGGING_PIN A2 
#define DEFAULT_FRAMERATE 16666 // 30 FPS
#endif


// Attach a new CmdMessenger object to the default Serial port
extern CmdMessenger cmdMessenger;

// ----------------------------------------------------------------------------
// Variables used for behavioral control

extern int activatedWell; // used by maze logic

// Arduino input pins corresponding to food well pumps and beam breaks, 
//  actual definition is in each specific task file
extern const int nFoodWells;
extern int beamBreakPins[];
extern int beamBreakFlags[];
extern int pumpPins[];
extern int wellRewardCounts[];

// Codes for no wells and multiple wells triggered
#define NONE -1
#define MULTI_WELL -2

extern const char maze_type[];

// ----------------------------------------------------------------------------
// Common functions
void triggerCamera(void); // Timer callback funtion which triggers camera frame

void LogFrameTrigger (void); // Function to log frame trigger times

void CheckBeamBreaks(void); // Called from main loop to check beam breaks
                            //  eventually replace with IO interrupt

void LogWellVisit (void); // Called from main loop to transmit timestamps for foodwell visits

void RewardWell(int whichOne); // Called from main loop when a well is to be rewarded
                               //  (starts appropriate pump)

void DoDispense(); // Called from main loop to turn pumps off when timer is expired

void CommonMazeSetup(); // Called from within setup() function


// ----------------------------------------------------------------------------
// Callbacks for IO

// Callbacks define on which received commands we take action
void attachCommandCallbacks();

// Called when a received command has no attached function
void OnUnknownCommand();

// Report version
void OnVersion();

// Callback function that sets camera frame period
void OnSetFramePeriod();

// Callback function that sets camera frame trigger pin
void OnSetTriggerPin();

// Callback function that sets pin which mirrors camera frame trigger when logging
void OnSetLoggingPin();

// Callback function that starts or stops camera triggering
void OnEnableTrigger();

// Callback function that starts or stops logging of camera frame times
//  our assumption is that this is always called while triggering is disabled
void OnEnableTriggerLogging();

// Callback function to test well logic
void OnFakeBeamBreak();

// Callback function to clear well counters
void OnClearWellLog();

// Callback function to query pin sey up
void OnQueryPins();

// Callback function to report well counters
void OnQueryWellLog();


#endif

