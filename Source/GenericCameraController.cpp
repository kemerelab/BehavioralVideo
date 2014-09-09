#include "GenericCameraController.h"
#include <QDebug>

GenericCameraController::GenericCameraController(QObject *parent) :
    QObject(parent)
{
    numCameras = 0;
}

/*void GenericCameraController::addCamera()
{
    numCameras++;
}*/

GenericCameraController::~GenericCameraController()
{

}

/*
void GenericCameraController::savingStateMachine(QString filename)
{
    qDebug() << "Saving state (QString) " << writingState;
    switch (writingState) {
    case NOT_WRITING:
        writingState = INITIALIZING_VIDEOWRITERS;
        aggregated = 0;
        emit initializeWriting(filename);
        break;
    default:
        qDebug() << "Unexpected signal received in camera controller state machine";
    }
}

void GenericCameraController::savingStateMachine(void)
{
    switch (writingState) {
    case INITIALIZING_VIDEOWRITERS:
        aggregated++;
        if (aggregated == numCameras) {
            writingState = PRE_SAVE_STOPPING_CAMERA;
            aggregated = 0;
            emit stopCapture();
        }
        break;
    case PRE_SAVE_STOPPING_CAMERA:
        aggregated++;
        if (aggregated == numCameras) {
            writingState = PRE_SAVE_TRIGGERING;
            aggregated = 0;
            startTrigger(false); // assume this takes time
            emit startCapture(false);
        }
        break;
    case PRE_SAVE_STARTING_CAMERA:
        aggregated++;
        if (aggregated == numCameras) {
            writingState = PRE_SAVE_TRIGGERING;
            aggregated = 0;
        }
        emit updateMenus(READY_TO_WRITE);
        break;
    case PRE_SAVE_TRIGGERING:
        writingState = PRE_SAVE_PAUSE_TRIGGERING;
        stopTrigger(); // assume this takes time
    case PRE_SAVE_PAUSE_TRIGGERING:
        writingState = STARTING_VIDEOWRITERS;
        aggregated = 0;
        emit startWriting();
        break;
    case STARTING_VIDEOWRITERS:
        aggregated++;
        if (aggregated == numCameras) {
            writingState = SAVE_TRIGGERING;
            aggregated = 0;
            emit startCapture(true);
        }
        break;
    case SAVE_TRIGGERING:
        aggregated++; //
        if (aggregated == numCameras) {
            writingState = POST_SAVE_PAUSE_TRIGGERING;
            aggregated = 0;
            startTrigger(true); // assume this takes some time
        }
        emit updateMenus(CURRENTLY_WRITING);
        break;
    case POST_SAVE_PAUSE_TRIGGERING:
        writingState = POST_SAVE_STOPPING_CAMERA;
        stopTrigger(); // assume this takes time
        aggregated = 0;
        emit stopCapture();
    case POST_SAVE_STOPPING_CAMERA:
        aggregated++;
        if (aggregated == numCameras) {
            writingState = STOPPING_VIDEOWRITERS;
            aggregated = 0;
            emit stopWriting();
        }
        break;
    case POST_SAVE_STARTING_CAMERA:
        aggregated++;
        if (aggregated == numCameras) {
            writingState = NOT_WRITING;
            aggregated = 0;
            emit startCapture(false);
        }
        emit updateMenus(NOT_SAVING);
        break;
    default:
        qDebug() << "Unexpected signal received when in state  (NOT_WRITING)";
    }
}
*/
