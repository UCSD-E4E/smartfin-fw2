#include "fileCLI.hpp"

#include "base85.h"
#include "base64.h"
#include "conio.hpp"
#include "utils.hpp"
#include "system.hpp"
#if SF_UPLOAD_ENCODING == SF_UPLOAD_BASE85
/**
 * How many bytes to store chunks of data in on the SPI flash.
 * 
 * 496 * 5/4 (base85 encoding compression rate) = 620 which is less than the 622
 * bytes which is the maximum size of publish events.
 */
#define FILE_BLOCK_SIZE   496
#elif SF_UPLOAD_ENCODING == SF_UPLOAD_BASE64 || SF_UPLOAD_ENCODING == SF_UPLOAD_BASE64URL
/**
 * How many bytes to store chunks of data in on the SPI flash.
 * 
 * 466 * 4/3 (base64 encoding compression rate) = 621 which is less than the 622
 * bytes which is the maximum size of publish events.
 */
#define FILE_BLOCK_SIZE   466
#endif
typedef struct menu_
{
    const char cmd;
    void (FileCLI::*fn)(void);
} menu_t;

menu_t fsExplorerMenu[] = 
{
    {'N', &FileCLI::doNextFile},
    {'C', &FileCLI::copyFile},
    {'R', &FileCLI::dumpBase85},
    {'U', &FileCLI::dumpDecimal},
    {'H', &FileCLI::dumpHex},
    {'D', &FileCLI::deleteFile},
    {'E', &FileCLI::exit},
    {'\0', NULL}
};

static menu_t* FileCLI_findCommand(const char* const cmd, menu_t* menu)
{
    for(;menu->cmd; menu++)
    {
        if(menu->cmd == cmd[0])
        {
            return menu;
        }
    }
    return NULL;
}


FileCLI::FileCLI(void){

}

void FileCLI::execute(void)
{
    char inputBuffer[FILE_CLI_INPUT_BUFFER_LEN];
    menu_t* cmd;
    void (FileCLI::*fn)(void);

    this->loopApp = 1;
    this->loopFile = 1;
    SF_OSAL_printf("Press N to go to next file, C to copy, R to read it out ("
        #if SF_UPLOAD_ENCODING == SF_UPLOAD_BASE85
        "base85"
        #elif SF_UPLOAD_ENCODING == SF_UPLOAD_BASE64
        "base64"
        #elif SF_UPLOAD_ENCODING == SF_UPLOAD_BASE64URL
        "base64url"
        #endif
        "), U to read it out (uint8_t), D to delete, E to exit\n");
    
    if(!pSystemDesc->pFileSystem->opendir("", &this->dir))
    {
        SF_OSAL_printf("*SPI Flash opendir fail\n");
        return;
    }

    while(this->loopApp)
    {
        if(!pSystemDesc->pFileSystem->readdir(&this->dir, &this->dirEnt))
        {
            break;
        }
        this->loopFile = 1;

        SF_OSAL_printf("%s\t%d\n", this->dirEnt.name, this->dirEnt.size);
        while(this->loopFile && this->loopApp)
        {
            SF_OSAL_printf(":>");
            getline(inputBuffer, FILE_CLI_INPUT_BUFFER_LEN);
            cmd = FileCLI_findCommand(inputBuffer, fsExplorerMenu);
            if(!cmd)
            {
                SF_OSAL_printf("Unknown command\n");
            }
            else
            {
                fn = cmd->fn;
                ((*this).*fn)();
            }
        }

    }
    if(this->loopApp)
    {
        SF_OSAL_printf("End of Directory\n");
    }
    pSystemDesc->pFileSystem->closedir(&this->dir);
}

void FileCLI::doNextFile(void)
{
    this->loopFile = 0;
}

void FileCLI::copyFile(void)
{
    int i;
    char copyFileName[33];
    SpiffsParticleFile copyFile;
    SpiffsParticleFile binFile;
    for(i = 0; i < 100; i++)
    {
        snprintf(copyFileName, 32, "copy_%02d", i);

        copyFile = pSystemDesc->pFileSystem->openFile(copyFileName, SPIFFS_O_RDONLY);

        if(copyFile.isValid())
        {
            copyFile.close();
            continue;
        }
        else
        {
            copyFile.close();
            break;
        }
    }
    copyFile = pSystemDesc->pFileSystem->openFile(copyFileName, SPIFFS_O_RDWR | SPIFFS_O_CREAT);

    binFile = pSystemDesc->pFileSystem->openFile((char*)this->dirEnt.name, SPIFFS_O_RDONLY);
    binFile.lseek(0, SPIFFS_SEEK_SET);

    while(!binFile.eof())
    {
        copyFile.write(binFile.read());
    }
    copyFile.flush();
    copyFile.close();
    SF_OSAL_printf("File Copied\n");
}

void FileCLI::dumpBase85(void)
{
    SpiffsParticleFile binFile;
    size_t numBytesToEncode;
    uint8_t dataBuffer[512];
    char encodedBuffer[1024];
    size_t encodedLen = 0;
    size_t nPackets = 0;
    size_t totalEncodedLen = 0;

    binFile = pSystemDesc->pFileSystem->openFile((char*) this->dirEnt.name, SPIFFS_O_RDONLY);
    binFile.lseek(0, SPIFFS_SEEK_SET);

    SF_OSAL_printf("Publish Header: %s-%s\n", pSystemDesc->deviceID, this->dirEnt.name);

    while(!binFile.eof())
    {
        numBytesToEncode = binFile.readBytes((char*) dataBuffer, FILE_BLOCK_SIZE);

        #if SF_UPLOAD_ENCODING == SF_UPLOAD_BASE85
        totalEncodedLen += bintob85(encodedBuffer, dataBuffer, numBytesToEncode) - encodedBuffer;
        #elif SF_UPLOAD_ENCODING == SF_UPLOAD_BASE64
        encodedLen = 1024;
        b64_encode(dataBuffer, numBytesToEncode, encodedBuffer, &encodedLen);
        totalEncodedLen += encodedLen;
        #elif SF_UPLOAD_ENCODING == SF_UPLOAD_BASE64URL
        encodedLen = 1024;
        urlsafe_b64_encode(dataBuffer, numBytesToEncode, encodedBuffer, &encodedLen);
        totalEncodedLen += encodedLen;
        #endif
        
        SF_OSAL_printf("%s\n", encodedBuffer);
        nPackets++;
    }
    SF_OSAL_printf("\n");
    SF_OSAL_printf("%d chars of base85 data\n", totalEncodedLen);
    SF_OSAL_printf("%d packets\n", nPackets);
    binFile.close();
}

void FileCLI::dumpDecimal(void)
{
    SpiffsParticleFile binFile;
    
    binFile = pSystemDesc->pFileSystem->openFile((char*) this->dirEnt.name, SPIFFS_O_RDONLY);
    
    SF_OSAL_printf("Publish Header: %s-%s\n", pSystemDesc->deviceID, this->dirEnt.name);

    while(!binFile.eof())
    {
        SF_OSAL_printf("%d,", binFile.read() & 0xFFu);
    }
    SF_OSAL_printf("\n");
    binFile.close();
}

void FileCLI::dumpHex(void)
{
    const size_t rowLen = 32;
    SpiffsParticleFile binFile;
    size_t i;
    uint8_t byteBuffer[rowLen];
    size_t nBytes;
    uint16_t offset = 0;

    binFile = pSystemDesc->pFileSystem->openFile((char*) this->dirEnt.name, SPIFFS_O_RDONLY);

    SF_OSAL_printf("Publish Header: %s-%s\n", pSystemDesc->deviceID, this->dirEnt.name);

    while(!binFile.eof())
    {
        nBytes = binFile.readBytes((char*) byteBuffer, rowLen);
        SF_OSAL_printf("%04x: ", offset);
        for(i = 0; i < rowLen; i += 2)
        {
            SF_OSAL_printf("%02x%02x ", byteBuffer[i], byteBuffer[i + 1]);
        }
        SF_OSAL_printf("\n");
        offset += nBytes;
    }
    binFile.close();
}

void FileCLI::deleteFile(void)
{
    SpiffsParticleFile binFile;

    binFile = pSystemDesc->pFileSystem->openFile((char*) this->dirEnt.name, SPIFFS_O_RDWR);
    if(binFile.remove() == SPIFFS_OK)
    {
        SF_OSAL_printf("*file deleted\n");
    }
    else
    {
        SF_OSAL_printf("*deletion failed\n");
    }
}

void FileCLI::exit(void)
{
    this->loopApp = 0;
}

FileCLI::~FileCLI(void)
{

}