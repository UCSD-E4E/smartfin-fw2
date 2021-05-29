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
    int putBytes(const void* pData, size_t nBytes);

    template <typename T> int putData(T& data)
    {
        return this->putBytes(&data, sizeof(T));
    };

    private:
    char currentSessionName[REC_SESSION_NAME_MAX_LEN + 1];
    uint8_t dataBuffer[REC_MEMORY_BUFFER_SIZE];
    uint32_t dataIdx;
    Deployment* pSession;

    void getSessionName(char* fileName);
};

#endif