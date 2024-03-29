#include "recorder.hpp"

#include "system.hpp"
#include "deploy.hpp"
#include "conio.hpp"

#define REC_DEBUG
static int REC_getNumFiles(void);

/**
 * @brief Initializes the Recorder to an idle state
 * 
 * @return int 1 if successful, otherwise 0
 */
int Recorder::init(void)
{
    memset(this->lastSessionName, 0, REC_SESSION_NAME_MAX_LEN + 1);
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
    if (!pSystemDesc->pFileSystem->opendir("", &dir))
    {
        return 0;
    }
    while (pSystemDesc->pFileSystem->readdir(&dir, &dirEntry))
    {
        if(dirEntry.name[0] != '.' && dirEntry.name[0] != '_')
        {
            pSystemDesc->pFileSystem->closedir(&dir);
            return 1;
        }
    }
    pSystemDesc->pFileSystem->closedir(&dir);
    return 0;
}

int Recorder::getNumFiles(void)
{
    return REC_getNumFiles();
}
#define REC_DIR_TREE_SIZE 16

typedef struct REC_dirTree_
{
    char filename[SPIFFS_OBJ_NAME_LEN];
    uint8_t initialized;
} REC_dirTree_t;

static REC_dirTree_t REC_dirTree[REC_DIR_TREE_SIZE];
static uint8_t REC_dirTreeInitialized = 0;
static int REC_dirTreeSkipped = 0;

static int REC_getNumFiles(void)
{
    spiffs_DIR dir;
    spiffs_dirent dirEntry;
    int i = 0;

    if (!pSystemDesc->pFileSystem->opendir("", &dir))
    {
        return -1;
    }

    while (pSystemDesc->pFileSystem->readdir(&dir, &dirEntry))
    {
        i++;
    }
    pSystemDesc->pFileSystem->closedir(&dir);
    return i;
}

// static int REC_treeGetNumFiles(void)
// {
//     int i = 0;
//     int numFiles = 0;
//     for (i = 0; i < REC_DIR_TREE_SIZE; i++)
//     {
//         numFiles += (REC_dirTree[i].initialized != 0);
//     }
//     return numFiles;
// }

static int REC_initializeTree(void)
{
    int nFiles = REC_getNumFiles();
    int skipFiles = 0;
    spiffs_DIR dir;
    spiffs_dirent dirEntry;
    int i = 0;

    memset(REC_dirTree, 0, sizeof(REC_dirTree_t) * REC_DIR_TREE_SIZE);

    if (nFiles > REC_DIR_TREE_SIZE)
    {
        skipFiles = nFiles - REC_DIR_TREE_SIZE;
        REC_dirTreeSkipped = 1;
    }

    if (!pSystemDesc->pFileSystem->opendir("", &dir))
    {
        SF_OSAL_printf("Failed to open directory\n");
        return 1;
    }
    for (i = 0; i < skipFiles; i++)
    {
        if (!pSystemDesc->pFileSystem->readdir(&dir, &dirEntry))
        {
            SF_OSAL_printf("Failed to skip initial files\n");
            return 1;
        }
    }

    for (i = 0; pSystemDesc->pFileSystem->readdir(&dir, &dirEntry) && i < 
        REC_DIR_TREE_SIZE; i++)
    {
        strcpy(REC_dirTree[i].filename, (char *)dirEntry.name);
        REC_dirTree[i].initialized = 1;
    }
    pSystemDesc->pFileSystem->closedir(&dir);
    REC_dirTreeInitialized = 1;
    SF_OSAL_printf("Tree Initialized\n");
    return 0;
}

int Recorder::openLastSession(Deployment &session, char* pName)
{
    int fileIdx;

    if (REC_initializeTree())
    {
        SF_OSAL_printf("Failed to initialize tree\n");
        return 1;
    }
    for(int i = 0; i < REC_DIR_TREE_SIZE; i++)
    {
        SF_OSAL_printf("%d: %32s %d\n", i, REC_dirTree[i].filename, 
            REC_dirTree[i].initialized);
    }

    fileIdx = REC_DIR_TREE_SIZE - 1;
    do
    {
        if (REC_dirTree[fileIdx].initialized && REC_dirTree[fileIdx].filename[0] != '.')
        {
            SF_OSAL_printf("Trying to open %d %32s %d\n", fileIdx, REC_dirTree[fileIdx].filename, REC_dirTree[fileIdx].initialized);
            if (!session.open(REC_dirTree[fileIdx].filename, Deployment::RDWR))
            {
#ifdef REC_DEBUG
                SF_OSAL_printf("REC::GLP open %s fail\n", REC_dirTree[fileIdx].filename);
#endif
                return 1;
            }
            else
            {
#ifdef REC_DEBUG
                SF_OSAL_printf("REC::GLP open %s success\n", REC_dirTree[fileIdx].filename);
                SF_OSAL_printf("Length: %d\n", session.getLength());

#endif
            }

            if (session.getLength() == 0)
            {
                SF_OSAL_printf("No bytes, removing\n");
                session.remove();
                REC_dirTree[fileIdx].initialized = 0;
                session.close();
            }
            else
            {
                strcpy(pName, REC_dirTree[fileIdx].filename);
                return 0;
            }
        }
    }while(fileIdx-- > 0);

    SF_OSAL_printf("Failed to find session\n");
    return 1;
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
int Recorder::getLastPacket(void *pBuffer, size_t bufferLen, char *pName, size_t nameLen)
{
    Deployment &session = Deployment::getInstance();
    int newLength;
    int bytesRead;
    char name[SPIFFS_OBJ_NAME_LEN];

    if (this->openLastSession(session, name))
    {
        memset(this->currentSessionName, 0, REC_SESSION_NAME_MAX_LEN + 1);
        return -1;
    }

    newLength = session.getLength() - bufferLen;
    session.seek(newLength);
    bytesRead = session.read(pBuffer, bufferLen);
    snprintf((char *)pName, nameLen, "Sfin-%s-%s-%d", pSystemDesc->deviceID,
             name, newLength / REC_MAX_PACKET_SIZE);
    session.close();
    strcpy(this->lastSessionName, name);
    return bytesRead;
}

/**
 * @brief Trims the last block with specified length from the recorder
 * 
 * @param len Length of block to trim
 * @return int 1 if successful, otherwise 0
 */
int Recorder::popLastPacket(size_t len)
{
    // spiffs_DIR dir;
    // spiffs_dirent dirEntry;
    // int i = 0;
    Deployment &session = Deployment::getInstance();
    int newLength;

    // have last file in dirEntry
    if (!session.open(this->lastSessionName, Deployment::RDWR))
    {
#ifdef REC_DEBUG
        SF_OSAL_printf("REC::TRIM - Fail to open\n");
#endif
        return 0;
    }

    newLength = session.getLength() - len;
    if (newLength <= 0)
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
void Recorder::setSessionName(const char *const sessionName)
{
    memset(this->currentSessionName, 0, REC_SESSION_NAME_MAX_LEN + 1);
    strncpy(this->currentSessionName, sessionName, REC_SESSION_NAME_MAX_LEN);
    SF_OSAL_printf("Setting session name to %s\n", this->currentSessionName);
}

/**
 * @brief Opens a session and configures the Recorder to record data
 * 
 * @return int 1 if successful, otherwise 0
 */
int Recorder::openSession(const char *const sessionName)
{
    memset(this->currentSessionName, 0, REC_SESSION_NAME_MAX_LEN + 1);
    if (sessionName)
    {
        strncpy(this->currentSessionName, sessionName, REC_SESSION_NAME_MAX_LEN);
    }
    this->pSession = &Deployment::getInstance();
    if (!this->pSession->open("__temp", Deployment::WRITE))
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
    spiffs_stat stat;

    if (NULL == this->pSession)
    {
        SF_OSAL_printf("REC::CLOSE Already closed\n");
        return 1;
    }

    // flush buffer
    for (; this->dataIdx < REC_MAX_PACKET_SIZE; this->dataIdx++)
    {
        this->dataBuffer[this->dataIdx] = 0;
    }
    this->pSession->write(this->dataBuffer, REC_MAX_PACKET_SIZE);

    this->pSession->close();
    this->getSessionName(fileName);
    pSystemDesc->pFileSystem->rename("__temp", fileName);
#ifdef REC_DEBUG
    SF_OSAL_printf("Saving as %s\n", fileName);
    pSystemDesc->pFileSystem->stat(fileName, &stat);
    SF_OSAL_printf("Saved %u bytes\n", stat.size);
#endif
    memset(this->dataBuffer, 0, REC_MEMORY_BUFFER_SIZE);
    this->dataIdx = 0;
    return 1;
}

void Recorder::getSessionName(char *pFileName)
{
    uint32_t i;
    char tempFileName[REC_SESSION_NAME_MAX_LEN + 1];
    SpiffsParticleFile fh;

    if (this->currentSessionName[0])
    {
        strcpy(pFileName, this->currentSessionName);
        return;
    }
    for (i = 0; i < 100; i++)
    {
        snprintf(tempFileName, REC_SESSION_NAME_MAX_LEN, "000000_temp_%02lu", i);
        fh = pSystemDesc->pFileSystem->openFile(tempFileName, SPIFFS_O_RDONLY);
        if (fh.isValid())
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

int Recorder::putBytes(const void *pData, size_t nBytes)
{
    if (NULL == this->pSession)
    {
        return 0;
    }
    if (nBytes > (REC_MAX_PACKET_SIZE - this->dataIdx))
    {
        // data will not fit, flush and clear
        // pad 0
        for (; this->dataIdx < REC_MAX_PACKET_SIZE; this->dataIdx++)
        {
            this->dataBuffer[this->dataIdx] = 0;
        }
        SF_OSAL_printf("Flushing\n");
        this->pSession->write(this->dataBuffer, REC_MAX_PACKET_SIZE);

        memset(this->dataBuffer, 0, REC_MEMORY_BUFFER_SIZE);
        this->dataIdx = 0;
    }

    // data guaranteed to fit
    SF_OSAL_printf("Putting %u bytes\n", nBytes);
    memcpy(&this->dataBuffer[this->dataIdx], pData, nBytes);
    this->dataIdx += nBytes;
    return 1;
}