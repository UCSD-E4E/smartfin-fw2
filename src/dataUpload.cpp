#include "dataUpload.hpp"

#include "Particle.h"
#include "deploy.hpp"
#include "system.hpp"
#include "conio.hpp"
#include "product.hpp"
#include "base85.h"
#include "sleepTask.hpp"


void DataUpload::init(void)
{
    SF_OSAL_printf("Entering SYSTEM_STATE_DATA_UPLOAD\n");

    this->initSuccess = 0;
    Particle.connect();
    os_thread_yield();
    this->initSuccess = 1;
}

STATES_e DataUpload::run(void)
{
    uint8_t dataEncodeBuffer[DATA_UPLOAD_MAX_BLOCK_LEN];
    char dataPublishBuffer[DATA_UPLOAD_MAX_UPLOAD_LEN];
    char publishName[DU_PUBLISH_ID_NAME_LEN + 1];
    int nBytesToEncode;
    int nBytesToSend;
    system_tick_t lastSendTime = 0;
    uint8_t uploadAttempts;

    if(!this->initSuccess)
    {
        SF_OSAL_printf("Failed to init\n");
        return STATE_DEEP_SLEEP;
    }

    //checks 3G flag
    bool no_upload_flag;
    pSystemDesc->pNvram->get(NVRAM::NO_UPLOAD_FLAG, no_upload_flag);
    if (no_upload_flag) {
        SF_OSAL_printf("no_upload mode set: entering sleep state\n");
        return STATE_CHARGE;
        //this can go to state_charge if we want to not save battery...
    }

    while(1)
    {
        // Power is most important.  If we don't have enough power, don't even
        // peek at the recorder
        SF_OSAL_printf("Voltage: %f\n", pSystemDesc->pBattery->getVCell());
        if(pSystemDesc->pBattery->getVCell() < SF_BATTERY_UPLOAD_VOLTAGE)
        {
            SF_OSAL_printf("Battery low\n");
            return STATE_DEEP_SLEEP;
        }


        // Do we have something to publish to begin with?  If not, save power
        if(!pSystemDesc->pRecorder->hasData())
        {
            SF_OSAL_printf("No data to transmit\n");
            return STATE_DEEP_SLEEP;
        }

        // set up connection

        if(!waitFor(Particle.connected, SF_CELL_SIGNAL_TIMEOUT_MS))
        {
            SF_OSAL_printf("Fail to connect\n");
            if(SleepTask::getBootBehavior() != SleepTask::BOOT_BEHAVIOR_UPLOAD_REATTEMPT)
            {
                SleepTask::setBootBehavior(SleepTask::BOOT_BEHAVIOR_UPLOAD_REATTEMPT);
                uploadAttempts = DU_UPLOAD_MAX_REATTEMPTS;
                pSystemDesc->pNvram->put(NVRAM::UPLOAD_REATTEMPTS, uploadAttempts);
            }
            else
            {
                pSystemDesc->pNvram->get(NVRAM::UPLOAD_REATTEMPTS, uploadAttempts);
                uploadAttempts--;
                if(uploadAttempts == 0)
                {
                    SleepTask::setBootBehavior(SleepTask::BOOT_BEHAVIOR_NORMAL);
                }
                else
                {
                    SleepTask::setBootBehavior(SleepTask::BOOT_BEHAVIOR_UPLOAD_REATTEMPT);
                    pSystemDesc->pNvram->put(NVRAM::UPLOAD_REATTEMPTS, uploadAttempts);
                }
            }
            return STATE_DEEP_SLEEP;
        }

        // connected, but maybe we went into the water
        if(pSystemDesc->pWaterSensor->getLastStatus() == WATER_SENSOR_HIGH_STATE)
        {
            SF_OSAL_printf("In the water!\n");
            return STATE_SESSION_INIT;
        }

        // connected, not in the water, publish!
        while(millis() - lastSendTime < DATA_UPLOAD_MIN_PUBLISH_TIME_MS)
        {
            os_thread_yield();
        }

        // have something to publish, grab and encode.
        memset(dataEncodeBuffer, 0, DATA_UPLOAD_MAX_BLOCK_LEN);
        nBytesToEncode = pSystemDesc->pRecorder->getLastPacket(dataEncodeBuffer, DATA_UPLOAD_MAX_BLOCK_LEN, publishName, DU_PUBLISH_ID_NAME_LEN);
        if(-1 == nBytesToEncode)
        {
            SF_OSAL_printf("Failed to retrive data\n");
            return STATE_CLI;
        }

        SF_OSAL_printf("Publish ID: %s\n", publishName);
        if(nBytesToEncode % 4 != 0)
        {
            nBytesToEncode += 4 - (nBytesToEncode % 4);
        }
        SF_OSAL_printf("Got %d bytes to encode\n", nBytesToEncode);

        memset(dataPublishBuffer, 0, DATA_UPLOAD_MAX_UPLOAD_LEN);
        nBytesToSend = (bintob85(dataPublishBuffer, dataEncodeBuffer, nBytesToEncode) - dataPublishBuffer);
        SF_OSAL_printf("Got %d bytes to upload\n", nBytesToSend);

        if(!Particle.connected())
        {
            // we're not connected!  abort and try again
            continue;
        }
        if(!Particle.publish(publishName, dataPublishBuffer, PRIVATE | WITH_ACK))
        {
            SF_OSAL_printf("Failed to upload data!\n");
            continue;
        }
        SF_OSAL_printf("Uploaded record %s\n", dataPublishBuffer);
        Particle.process();
        lastSendTime = millis();
        
        //Creates a file to save 4G data as it gets deleted from original file.
        SpiffsParticleFile bin_file = pSystemDesc->pFileSystem->openFile("Saved4GData", SPIFFS_O_RDWR | SPIFFS_O_CREAT);
        for (int j = 0; j < nBytesToSend; j++)
        {
            bin_file.write(dataPublishBuffer[j]);
        }

        if(!pSystemDesc->pRecorder->popLastPacket(nBytesToEncode))
        {
            SF_OSAL_printf("Failed to trim!");
            return STATE_CLI;
        }
        
    }
}

void DataUpload::exit(void)
{
    Cellular.off();
}

STATES_e DataUpload::exitState(void)
{
    // if in water, switch to data collection
    if(pSystemDesc->pWaterSensor->getLastStatus() == WATER_SENSOR_HIGH_STATE)
    {
        return STATE_SESSION_INIT;
    }
    return STATE_DEEP_SLEEP;

}