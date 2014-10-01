/*
  Interface specification for Behavior Control Arduino code
  to communicate using "CmdMessenger" library with BehavioralVideo
 */

#ifndef BEHAVIOR_INTERFACE_COMMANDS_H
#define BEHAVIOR_INTERFACE_COMMANDS_H

// This is the list of recognized commands. These can be commands that can either be sent or received. 
// In order to receive, attach a callback function to these events
enum CommandEnum
{
  kStatus,           // Generic command for status messages
  kEvent,            // Command for well events
  kClearWellLog,     // Command to clear well counters
  kSetFramePeriod,   // Command to set camera triggering frame rate (argument: period in microseconds)
  kSetTriggerPin,    // Command to set camera trigger pin (argument: pin number)
  kEnableTrigger,    // Command to start/stop triggering (argument: true=on, false=off)
  kEnableTriggerLogging, // Command to enable trigger *logging*
  kSetEventPin,      // Command to set IO event pin (argument: pin number)
  kQueryWellLog,     // Command to ask for well log
  kQueryPins,        // Command to ask for IO pin setup
  kFakeBeamBreak,    // Command to fake a well visit (argument: character A B or C)
  kVersion,          // Command to ask for version info
};

#endif

