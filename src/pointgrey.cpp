#include "pointgrey.h"

PointGrey::PointGrey(QObject *parent) :
    QObject(parent)
{
}

//=============================================================================
// $Id: SaveImageToAviEx.cpp,v 1.10 2009-12-08 18:58:36 soowei Exp $
//=============================================================================

void PrintBuildInfo()
{
    FC2Version fc2Version;
    Utilities::GetLibraryVersion( &fc2Version );
    char version[128];
    sprintf(
        version,
        "FlyCapture2 library version: %d.%d.%d.%d\n",
        fc2Version.major, fc2Version.minor, fc2Version.type, fc2Version.build );

    printf( "%s", version );

    char timeStamp[512];
    sprintf( timeStamp, "Application build date: %s %s\n\n", __DATE__, __TIME__ );

    printf( "%s", timeStamp );
}

void PrintCameraInfo( CameraInfo* pCamInfo )
{
    printf(
        "\n*** CAMERA INFORMATION ***\n"
        "Serial number - %u\n"
        "Camera model - %s\n"
        "Camera vendor - %s\n"
        "Sensor - %s\n"
        "Resolution - %s\n"
        "Firmware version - %s\n"
        "Firmware build time - %s\n\n",
        pCamInfo->serialNumber,
        pCamInfo->modelName,
        pCamInfo->vendorName,
        pCamInfo->sensorInfo,
        pCamInfo->sensorResolution,
        pCamInfo->firmwareVersion,
        pCamInfo->firmwareBuildTime );
}

void PrintError( Error error )
{
    error.PrintErrorTrace();
}

void SaveAviHelper(
    AviType aviType,
    std::vector<Image>& vecImages,
    std::string aviFileName,
    float frameRate)
{
    Error error;
    AVIRecorder aviRecorder;

    // Open the AVI file for appending images

    switch (aviType)
    {
    case UNCOMPRESSED:
        {
            AVIOption option;
            option.frameRate = frameRate;
            error = aviRecorder.AVIOpen(aviFileName.c_str(), &option);
        }
        break;
    case MJPG:
        {
            MJPGOption option;
            option.frameRate = frameRate;
            option.quality = 75;
            error = aviRecorder.AVIOpen(aviFileName.c_str(), &option);
        }
        break;
    case H264:
        {
            H264Option option;
            option.frameRate = frameRate;
            option.bitrate = 1000000;
            option.height = vecImages[0].GetRows();
            option.width = vecImages[0].GetCols();
            error = aviRecorder.AVIOpen(aviFileName.c_str(), &option);
        }
        break;
    }

    if (error != PGRERROR_OK)
    {
        PrintError(error);
        return;
    }

    printf( "\nAppending %d images to AVI file: %s ... \n", vecImages.size(), aviFileName.c_str() );
    for (int imageCnt = 0; imageCnt < vecImages.size(); imageCnt++)
    {
        // Append the image to AVI file
        error = aviRecorder.AVIAppend(&vecImages[imageCnt]);
        if (error != PGRERROR_OK)
        {
            PrintError(error);
            continue;
        }

        printf("Appended image %d...\n", imageCnt);
    }

    // Close the AVI file
    error = aviRecorder.AVIClose( );
    if (error != PGRERROR_OK)
    {
        PrintError(error);
        return;
    }
}

int RunCamera( PGRGuid guid )
{
    const int k_numImages = 100;

    Error error;
    Camera cam;

    // Connect to a camera
    error = cam.Connect(&guid);
    if (error != PGRERROR_OK)
    {
        PrintError(error);
        return -1;
    }

    // Get the camera information
    CameraInfo camInfo;
    error = cam.GetCameraInfo(&camInfo);
    if (error != PGRERROR_OK)
    {
        PrintError(error);
        return -1;
    }

    PrintCameraInfo(&camInfo);

    // Start capturing images
    printf( "Starting capture... \n" );
    error = cam.StartCapture();
    if (error != PGRERROR_OK)
    {
        PrintError(error);
        return -1;
    }

    std::vector<Image> vecImages;
    vecImages.resize(k_numImages);

    // Grab images
    Image rawImage;
    for ( int imageCnt=0; imageCnt < k_numImages; imageCnt++ )
    {
        error = cam.RetrieveBuffer(&rawImage);
        if (error != PGRERROR_OK)
        {
            printf("Error grabbing image %u\n", imageCnt);
            continue;
        }
        else
        {
            printf("Grabbed image %u\n", imageCnt);
        }

        vecImages[imageCnt].DeepCopy(&rawImage);
    }

    // Stop capturing images
    printf( "Stopping capture... \n" );
    error = cam.StopCapture();
    if (error != PGRERROR_OK)
    {
        PrintError(error);
        return -1;
    }

    // Check if the camera supports the FRAME_RATE property
    printf( "Detecting frame rate from camera... \n" );
    PropertyInfo propInfo;
    propInfo.type = FRAME_RATE;
    error = cam.GetPropertyInfo( &propInfo );
    if (error != PGRERROR_OK)
    {
        PrintError(error);
        return -1;
    }

    float frameRateToUse = 15.0f;
    if ( propInfo.present == true )
    {
        // Get the frame rate
        Property prop;
        prop.type = FRAME_RATE;
        error = cam.GetProperty( &prop );
        if (error != PGRERROR_OK)
        {
            PrintError(error);
        }
        else
        {
            // Set the frame rate.
            // Note that the actual recording frame rate may be slower,
            // depending on the bus speed and disk writing speed.
            frameRateToUse = prop.absValue;
        }
    }

    printf("Using frame rate of %3.1f\n", frameRateToUse);

    char aviFileName[512] = {0};

    sprintf(aviFileName, "SaveImageToAviEx-Uncompressed-%u", camInfo.serialNumber);
    SaveAviHelper(UNCOMPRESSED, vecImages, aviFileName, frameRateToUse);

    sprintf(aviFileName, "SaveImageToAviEx-Mjpg-%u", camInfo.serialNumber);
    SaveAviHelper(MJPG, vecImages, aviFileName, frameRateToUse);

    sprintf(aviFileName, "SaveImageToAviEx-h264-%u", camInfo.serialNumber);
    SaveAviHelper(H264, vecImages, aviFileName, frameRateToUse);

    // Disconnect the camera
    error = cam.Disconnect();
    if (error != PGRERROR_OK)
    {
        PrintError(error);
        return -1;
    }

    return 0;
}

int main(int /*argc*/, char** /*argv*/)
{
    PrintBuildInfo();

    Error error;

    // Since this application saves images in the current folder
    // we must ensure that we have permission to write to this folder.
    // If we do not have permission, fail right away.
    FILE* tempFile = fopen("test.txt", "w+");
    if (tempFile == NULL)
    {
       printf("Failed to create file in current folder.  Please check permissions.\n");
       return -1;
    }
    fclose(tempFile);
    remove("test.txt");

    BusManager busMgr;
    unsigned int numCameras;
    error = busMgr.GetNumOfCameras(&numCameras);
    if (error != PGRERROR_OK)
    {
        PrintError(error);
        return -1;
    }

    if ( numCameras < 1 )
    {
       printf( "No camera detected.\n" );
       return -1;
    }
    else
    {
       printf( "Number of cameras detected: %u\n", numCameras );
    }

    PGRGuid guid;
    error = busMgr.GetCameraFromIndex(0, &guid);
    if (error != PGRERROR_OK)
    {
       PrintError(error);
       return -1;
    }

    printf( "Running the first camera.\n" );
    RunCamera( guid );

    printf( "Done! Press Enter to exit...\n" );
    getchar();

    return 0;
}
