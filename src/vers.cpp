#include "vers.hpp"

#include "conio.hpp"



void VERS_printBanner(void)
{
    SF_OSAL_printf("Smartfin FW v%d.%d.%d.%d%s\r\n", FW_MAJOR_VERSION, FW_MINOR_VERSION, FW_PATCH_VERSION, FW_BUILD_NUM, FW_BRANCH);
    SF_OSAL_printf("FW Build: %s %s\n", __DATE__, __TIME__);
}