#include "vers.hpp"

#include "conio.hpp"

const char* BUILD_DATE = __DATE__;
const char* BUILD_TIME = __TIME__;


void VERS_printBanner(void)
{
    SF_OSAL_printf("Smartfin FW v%d.%d.%d.%d%s\r\n", FW_MAJOR_VERSION, FW_MINOR_VERSION, FW_PATCH_VERSION, FW_BUILD_NUM, FW_BRANCH);
    SF_OSAL_printf("FW Build: %s %s\n", BUILD_DATE, BUILD_TIME);
}

const char* VERS_getBuildDate(void)
{
    return BUILD_DATE;
}
const char* VERS_getBuildTime(void)
{
    return BUILD_TIME;
}
