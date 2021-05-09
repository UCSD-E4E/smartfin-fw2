#ifndef __RECORDER_HPP__
#define __RECORDER_HPP__

#include "SpiffsParticleRK.h"
#include <stddef.h>
#include "deploy.hpp"
#include "conio.hpp"

/**
 * @brief Maximum Session Name Length
 * 
 */
#define REC_SESSION_NAME_MAX_LEN 31

#define REC_MEMORY_BUFFER_SIZE  512
#define REC_MAX_PACKET_SIZE  496

class Recorder
{
    public:
    int init(void);
    int hasData(void);
    int getLastPacket(void* pBuffer, size_t bufferLen, char* pName, size_t nameLen);
    void resetPacketNumber(void);
    void incrementPacketNumber(void);
    int popLastPacket(size_t len);
    void setSessionName(const char* const);

    int openSession(const char* const depName);
    int closeSession(void);

    template <typename T> int putData(T& data)
    {
        if(NULL == this->pSession)
        {
            return 0;
        }
        if(sizeof(T) > (REC_MAX_PACKET_SIZE - this->dataIdx))
        {
            // data will not fit, flush and clear
            // pad 0
            for(; this->dataIdx < REC_MAX_PACKET_SIZE; this->dataIdx++)
            {
                this->dataBuffer[this->dataIdx] = 0;
            }
            this->pSession->write(this->dataBuffer, REC_MAX_PACKET_SIZE);

            memset(this->dataBuffer, 0, REC_MEMORY_BUFFER_SIZE);
            this->dataIdx = 0;
        }

        // data guaranteed to fit
        memcpy(&this->dataBuffer[this->dataIdx], &data, sizeof(T));
        this->dataIdx += sizeof(T);
        return 1;
    };

    private:
    int packetNumber;
    char currentSessionName[REC_SESSION_NAME_MAX_LEN + 1];
    uint8_t dataBuffer[REC_MEMORY_BUFFER_SIZE];
    uint32_t dataIdx;
    Deployment* pSession;

    void getSessionName(char* fileName);
};

#endif