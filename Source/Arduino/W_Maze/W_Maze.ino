/*
  Behavior Control 
 
 General template:
 Collects input from digital inputs (e.g., beam breaks over food wells),
 and controls digital outputs (e.g., syringe pumps or valves) to provide
 reward as appropriate.
 
 This file:
 Runs the logic for a w maze.
 Syringe pumps are active LOW
 
 */

#include "MazeControlFunctions.h"

const int nFoodWells = 3;
int beamBreakPins[nFoodWells] = {A4, A5, A3};
int beamBreakFlags[nFoodWells] = {0, 0, 0};
int pumpPins[nFoodWells] = {6, 7, A0};
int wellRewardCounts[nFoodWells] = {0, 0, 0};

const char maze_type[] = "W-Maze";

int lastWell, lastLastWell;

int wellA, wellB, wellC;

void setup() {

  CommonMazeSetup();
  
  // Specific W-Maze logic variables
  lastWell = NONE;
  lastLastWell = NONE;
  wellA = 0; wellB = 1; wellC = 2;
  
}

void loop() {   
  
  LogFrameTrigger();
  
  CheckBeamBreaks();
  
  // =================================================================================================
  // Task logic
  
  if (activatedWell == MULTI_WELL) {
    // something's wrong and multiple wells were activated
  } else if (activatedWell != NONE) {   
    if (activatedWell != lastWell) { // ignore repeated visits to a well
      LogWellVisit();
      
      if ( ( (lastLastWell == NONE) && (lastWell == NONE) ) ||
           ( (lastLastWell == NONE) && (lastWell == wellB) ) || 
           ( (lastLastWell == wellA) && (lastWell == wellB) && (activatedWell == wellC) ) ||
           ( (lastLastWell == wellC) && (lastWell == wellB) && (activatedWell == wellA) ) ||
           ( (lastWell == wellA) && (activatedWell == wellB) ) ||
           ( (lastWell == wellC) && (activatedWell == wellB) ) )
        RewardWell(activatedWell);

      lastLastWell = lastWell;
      lastWell = activatedWell;
    }
  }
  
  // =================================================================================================

  DoDispense();
  
  // Process incoming serial data, and perform callbacks
  cmdMessenger.feedinSerialData();  
}

