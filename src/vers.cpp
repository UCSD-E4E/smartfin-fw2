#include "vers.hpp"

#include "conio.hpp"

#define FW_MAJOR_VERSION    2
#define FW_MINOR_VERSION    1
#define FW_BUILD_NUM        0
#define FW_BRANCH           "NH0"

void VERS_printBanner(void)
{
    SF_OSAL_printf("Smartfin FW Version %d.%02d.%d%s\r\n", FW_MAJOR_VERSION, FW_MINOR_VERSION, FW_BUILD_NUM, FW_BRANCH);
    SF_OSAL_printf("FW Build: %s %s\n", __DATE__, __TIME__);
}