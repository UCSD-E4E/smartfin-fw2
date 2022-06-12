#include "vers.hpp"

#include "conio.hpp"

const char* BUILD_DATE = __DATE__;
const char* BUILD_TIME = __TIME__;

#if FW_MAJOR_VERSION > 7
#error Major Version exceeds field width!
#endif

#if FW_MINOR_VERSION > 63
#error Minor Version exceeds field width!
#endif

#if FW_BUILD_NUM > 127
#error Build Number exceeds field width!
#endif

void VERS_printBanner(void)
{
    SF_OSAL_printf("Smartfin FW v%d.%d.%d%s\r\n", FW_MAJOR_VERSION, FW_MINOR_VERSION, FW_BUILD_NUM, FW_BRANCH);
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
