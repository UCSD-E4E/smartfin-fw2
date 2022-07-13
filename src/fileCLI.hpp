#ifndef __FILECLI_H__
#define __FILECLI_H__

#include "SpiffsParticleRK.h"

#define FILE_CLI_INPUT_BUFFER_LEN   80

class FileCLI{
    public:
    FileCLI(void);
    void execute(void);
    ~FileCLI(void);
    spiffs_dirent dirEnt;

    void doNextFile(void);
    void copyFile(void);
    void dumpBase85(void);
    void dumpDecimal(void);
    void dumpHex(void);
    void dumpAscii(void);
    void deleteFile(void);
    void exit(void);

    private:
    int loopFile = 1;
    int loopApp = 1;
    spiffs_DIR dir;
    
};

#endif