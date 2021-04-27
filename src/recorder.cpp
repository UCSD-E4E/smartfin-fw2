#include "recorder.hpp"

#include "system.hpp"
#include "deploy.hpp"
#include "conio.hpp"

#define REC_DEBUG

int Recorder::init(void)
{
    return 1;
}

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

int Recorder::getLastBlock(void* pBuffer, size_t bufferLen, char* pName, size_t nameLen)
{
    spiffs_DIR dir;
    spiffs_dirent dirEntry;
    int i = 0;
    Deployment& deployment = Deployment::getInstance();
    int newLength;
    int bytesRead;

    if(!pSystemDesc->pFileSystem->opendir("", &dir))
    {
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
    if(!deployment.open((char*)dirEntry.name, Deployment::RDWR))
    {
        return -1;
    }

    while(deployment.getLength() == 0)
    {
        deployment.remove();
        deployment.close();
        if(!pSystemDesc->pFileSystem->opendir("", &dir))
        {
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
        if(!deployment.open((char*)dirEntry.name, Deployment::RDWR))
        {
            return -1;
        }
    }

    newLength = deployment.getLength() - bufferLen;
    deployment.seek(newLength);
    bytesRead = deployment.read(pBuffer, bufferLen);
    snprintf((char*) pName, nameLen, "Sfin-%s-%s-%d", pSystemDesc->deviceID, dirEntry.name, this->uploadNumber);
    deployment.close();
    return bytesRead;
}

/**
 * @brief Resets the Recorder upload number to 0
 * 
 */
void Recorder::resetUploadNumber(void)
{
    this->uploadNumber = 0;
}

/**
 * @brief Resets the Recorder upload number to 0
 * 
 */
void Recorder::incrementUploadNumber(void)
{
    this->uploadNumber++;
}

/**
 * @brief Trims the last block with specified length from the recorder
 * 
 * @param len Length of block to trim
 * @return int 1 if successful, otherwise 0
 */
int Recorder::trimLastBlock(size_t len)
{
    spiffs_DIR dir;
    spiffs_dirent dirEntry;
    int i = 0;
    Deployment& deployment = Deployment::getInstance();
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
    if(!deployment.open((char*)dirEntry.name, Deployment::RDWR))
    {
#ifdef REC_DEBUG
        SF_OSAL_printf("REC::TRIM - Fail to open\n");
#endif
        return 0;
    }

    while(deployment.getLength() == 0)
    {
        if(!deployment.remove())
        {
#ifdef REC_DEBUG
            SF_OSAL_printf("REC::TRIM - fail to remove empty file\n");
#endif
            return 0;
        }
        if(!deployment.close())
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
        if(!deployment.open((char*)dirEntry.name, Deployment::RDWR))
        {
#ifdef REC_DEBUG
            SF_OSAL_printf("REC::TRIM - fail to open\n");
#endif
            return 0;
        }
    }

    newLength = deployment.getLength() - len;
    if(newLength <= 0)
    {
        deployment.remove();
    }
    else
    {
        deployment.truncate(newLength);
    }
    deployment.close();

    return 1;
}

/**
 * @brief Set the current deployment name
 * 
 * @param depName Current name to set
 */
void Recorder::setDeploymentName(const char* const depName)
{
    memset(this->currentDeploymentName, 0, REC_DEPLOYMENT_NAME_MAX_LEN+1);
    strncpy(this->currentDeploymentName, depName, REC_DEPLOYMENT_NAME_MAX_LEN);
}