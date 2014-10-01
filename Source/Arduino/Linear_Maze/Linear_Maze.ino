/*
  Behavior Control 
 
 General template:
 Collects input from digital inputs (e.g., beam breaks over food wells),
 and controls digital outputs (e.g., syringe pumps or valves) to provide
 reward as appropriate.
 
 This file:
 Runs the logic for a linear (back-and-forth) maze.
 Syringe pumps are active LOW
 
 */

#include "MazeControlFunctions.h"

const int nFoodWells = 3;
int beamBreakPins[nFoodWells] = {A4, A5};
int beamBreakFlags[nFoodWells] = {0, 0};
int pumpPins[nFoodWells] = {6, 7};
int wellRewardCounts[nFoodWells] = {0, 0};

const char maze_type[] = "Linear Maze";

int lastWell;

void setup() {

  CommonMazeSetup();
  
  // Specific W-Maze logic variables
  lastWell = NONE;
  
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
      
      wellRewardCounts[activatedWell]++;
      RewardWell(activatedWell);

      lastWell = activatedWell;
    }
  }
  
  // =================================================================================================

  DoDispense();
  
  // Process incoming serial data, and perform callbacks
  cmdMessenger.feedinSerialData();  
}

