#ifndef __RECORDER_HPP__
#define __RECORDER_HPP__

#include "SpiffsParticleRK.h"
#include <stddef.h>

/**
 * @brief Maximum Deployment Name Length
 * 
 */
#define REC_DEPLOYMENT_NAME_MAX_LEN 31

class Recorder
{
    public:
    int init(void);
    int hasData(void);
    int getLastBlock(void* pBuffer, size_t bufferLen, char* pName, size_t nameLen);
    void resetUploadNumber(void);
    void incrementUploadNumber(void);
    int trimLastBlock(size_t len);
    void setDeploymentName(const char* const);
    private:
    int uploadNumber;
    char currentDeploymentName[REC_DEPLOYMENT_NAME_MAX_LEN + 1];
};

#endif