#include "recorder.hpp"

#include "system.hpp"
#include "deploy.hpp"
#include "conio.hpp"

#define REC_DEBUG

/**
 * @brief Initializes the Recorder to an idle state
 * 
 * @return int 1 if successful, otherwise 0
 */
int Recorder::init(void)
{
    return 1;
}

/**
 * @brief Checks if the Recorder has data to upload
 * 
 * @return int  1 if data exists, otherwise 0
 */
int Recorder::hasData(void)
{
    spiffs_DIR dir;
    spiffs_dirent dirEntry;
    if(!pSystemDesc->pFileSystem->opendir("", &dir))
    {
        return 0;
    }
    if(pSystemDesc->pFileSystem->readdir(&dir, &dirEntry))
    {
        pSystemDesc->pFileSystem->closedir(&dir);
        return 1;
    }
    pSystemDesc->pFileSystem->closedir(&dir);
    return 0;
}

/**
 * @brief Retrieves the last packet of data into pBuffer, and puts the session
 *  name into pName.
 * 
 * @param pBuffer Buffer to place last packet into
 * @param bufferLen Length of packet buffer
 * @param pName Buffer to place session name into
 * @param nameLen Length of name buffer
 * @return int -1 on failure, number of bytes placed into data buffer otherwise
 */
int Recorder::getLastPacket(void* pBuffer, size_t bufferLen, char* pName, size_t nameLen)
{
    spiffs_DIR dir;
    spiffs_dirent dirEntry;
    int i = 0;
    Deployment& session = Deployment::getInstance();
    int newLength;
    int bytesRead;

    if(!pSystemDesc->pFileSystem->opendir("", &dir))
    {
        #ifdef REC_DEBUG
        SF_OSAL_printf("REC::GLP opendir fail\n");
        #endif
        return -1;
    }

    while(pSystemDesc->pFileSystem->readdir(&dir, &dirEntry))
    {
        i++;
    }
    pSystemDesc->pFileSystem->closedir(&dir);

    if(0 == i)
    {
        // no files found, return 0
        return 0;
    }

    // have last file in dirEntry
    if(!session.open((char*)dirEntry.name, Deployment::RDWR))
    {
        #ifdef REC_DEBUG
        SF_OSAL_printf("REC::GLP open %s fail\n", (char*)dirEntry.name);
        #endif
        return -1;
    }
    else
    {
        #ifdef REC_DEBUG
        SF_OSAL_printf("REC::GLP open %s success\n", (char*) dirEntry.name);
        #endif
    }

    while(session.getLength() == 0)
    {
        session.remove();
        session.close();
        if(!pSystemDesc->pFileSystem->opendir("", &dir))
        {
            #ifdef REC_DEBUG
            SF_OSAL_printf("REC::GLP opendir 2 fail\n");
            #endif
            return -1;
        }

        while(pSystemDesc->pFileSystem->readdir(&dir, &dirEntry))
        {
            i++;
        }
        pSystemDesc->pFileSystem->closedir(&dir);

        if(0 == i)
        {
            // no files found, return 0
            return 0;
        }
        if(!session.open((char*)dirEntry.name, Deployment::RDWR))
        {
            #ifdef REC_DEBUG
            SF_OSAL_printf("REC::GLP open2 %s fail\n", (char*)dirEntry.name);
            #endif
            return -1;
        }
    }

    newLength = session.getLength() - bufferLen;
    session.seek(newLength);
    bytesRead = session.read(pBuffer, bufferLen);
    snprintf((char*) pName, nameLen, "Sfin-%s-%s-%d", pSystemDesc->deviceID, dirEntry.name, this->packetNumber);
    session.close();
    return bytesRead;
}

/**
 * @brief Resets the Recorder upload number to 0
 * 
 */
void Recorder::resetPacketNumber(void)
{
    this->packetNumber = 0;
}

/**
 * @brief Resets the Recorder upload number to 0
 * 
 */
void Recorder::incrementPacketNumber(void)
{
    this->packetNumber++;
}

/**
 * @brief Trims the last block with specified length from the recorder
 * 
 * @param len Length of block to trim
 * @return int 1 if successful, otherwise 0
 */
int Recorder::popLastPacket(size_t len)
{
    spiffs_DIR dir;
    spiffs_dirent dirEntry;
    int i = 0;
    Deployment& session = Deployment::getInstance();
    int newLength;

    if(!pSystemDesc->pFileSystem->opendir("", &dir))
    {
#ifdef REC_DEBUG
        SF_OSAL_printf("REC::TRIM - Failed to open dir\n");
#endif
        return 0;
    }

    while(pSystemDesc->pFileSystem->readdir(&dir, &dirEntry))
    {
        i++;
    }
    pSystemDesc->pFileSystem->closedir(&dir);

    if(0 == i)
    {
        // no files found, return 0
#ifdef REC_DEBUG
        SF_OSAL_printf("REC::TRIM - No files found\n");
#endif
        return 0;
    }

    // have last file in dirEntry
    if(!session.open((char*)dirEntry.name, Deployment::RDWR))
    {
#ifdef REC_DEBUG
        SF_OSAL_printf("REC::TRIM - Fail to open\n");
#endif
        return 0;
    }

    while(session.getLength() == 0)
    {
        if(!session.remove())
        {
#ifdef REC_DEBUG
            SF_OSAL_printf("REC::TRIM - fail to remove empty file\n");
#endif
            return 0;
        }
        if(!session.close())
        {
#ifdef REC_DEBUG
            SF_OSAL_printf("REC::TRIM - fail to close file\n");
#endif
            return 0;
        }
        if(!pSystemDesc->pFileSystem->opendir("", &dir))
        {
#ifdef REC_DEBUG
            SF_OSAL_printf("REC::TRIM - fail to open\n");
#endif
            return 0;
        }

        while(pSystemDesc->pFileSystem->readdir(&dir, &dirEntry))
        {
            i++;
        }
        pSystemDesc->pFileSystem->closedir(&dir);

        if(0 == i)
        {
            // no files found, return 0
#ifdef REC_DEBUG
            SF_OSAL_printf("REC::TRIM - No files found\n");
#endif
            return 0;
        }
        if(!session.open((char*)dirEntry.name, Deployment::RDWR))
        {
#ifdef REC_DEBUG
            SF_OSAL_printf("REC::TRIM - fail to open\n");
#endif
            return 0;
        }
    }

    newLength = session.getLength() - len;
    if(newLength <= 0)
    {
        session.remove();
    }
    else
    {
        session.truncate(newLength);
    }
    session.close();

    return 1;
}

/**
 * @brief Set the current session name
 * 
 * @param sessionName Current name to set
 */
void Recorder::setSessionName(const char* const sessionName)
{
    memset(this->currentSessionName, 0, REC_SESSION_NAME_MAX_LEN+1);
    strncpy(this->currentSessionName, sessionName, REC_SESSION_NAME_MAX_LEN);
    SF_OSAL_printf("Setting session name to %s\n", this->currentSessionName);
}

/**
 * @brief Opens a session and configures the Recorder to record data
 * 
 * @return int 1 if successful, otherwise 0
 */
int Recorder::openSession(const char* const sessionName)
{
    if(sessionName)
    {
        memset(this->currentSessionName, 0, REC_SESSION_NAME_MAX_LEN + 1);
        strncpy(this->currentSessionName, sessionName, REC_SESSION_NAME_MAX_LEN);
    }
    this->pSession = &Deployment::getInstance();
    if(!this->pSession->open("__temp", Deployment::WRITE))
    {
        this->pSession = 0;
        SF_OSAL_printf("REC::OPEN Fail to open\n");
        return 0;
    }
    else
    {
        memset(this->dataBuffer, 0, REC_MEMORY_BUFFER_SIZE);
        this->dataIdx = 0;
         SF_OSAL_printf("REC::OPEN opened %s\n", this->currentSessionName);
        return 1;
    }
}

/**
 * @brief Closes the current session
 * 
 * If the session was already closed, treated as success.
 * 
 * @return int  1 if successful, otherwise 0
 */
int Recorder::closeSession(void)
{
    char fileName[REC_SESSION_NAME_MAX_LEN + 1];
    if(NULL == this->pSession)
    {
        SF_OSAL_printf("REC::CLOSE Already closed\n");
        return 1;
    }

    this->pSession->close();
    this->getSessionName(fileName);
    pSystemDesc->pFileSystem->rename("__temp", fileName);
#ifdef REC_DEBUG
    SF_OSAL_printf("Saving as %s\n", fileName);
#endif
    memset(this->dataBuffer, 0, REC_MEMORY_BUFFER_SIZE);
    this->dataIdx = 0;
    return 1;
}

void Recorder::getSessionName(char* pFileName)
{
    uint32_t i;
    char tempFileName[REC_SESSION_NAME_MAX_LEN + 1];
    SpiffsParticleFile fh;

    if(this->currentSessionName[0])
    {
        strcpy(pFileName, this->currentSessionName);
        return;
    }
    for(i = 0; i < 100; i++)
    {
        snprintf(tempFileName, REC_SESSION_NAME_MAX_LEN, "000000_temp_%02lu", i);
        fh = pSystemDesc->pFileSystem->openFile(tempFileName, SPIFFS_O_RDONLY);
        if(fh.isValid())
        {
            fh.close();
            continue;
        }
        else
        {
            fh.close();
            break;
        }
    }
    strcpy(pFileName, tempFileName);
    return;
}