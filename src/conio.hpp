#ifndef __CONIO_HPP__
#define __CONIO_HPP__
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#define SF_OSAL_PRINTF_BUFLEN   1024
#define SF_OSAL_GETCH_USE_WDOG
#define SF_OSAL_GPS_GETLINE_ECHO
#define SF_OSAL_LINE_WIDTH  80
#ifdef __cplusplus
extern "C"
{
#endif
    typedef enum OSAL_LogPriority_
    {
        OSAL_LOG_PRIO_EMERG,
        OSAL_LOG_PRIO_ALERT,
        OSAL_LOG_PRIO_CRIT,
        OSAL_LOG_PRIO_ERR,
        OSAL_LOG_PRIO_WARN,
        OSAL_LOG_PRIO_INFO,
        OSAL_LOG_PRIO_DEBUG
    }OSAL_LogPriority_e;
    int SF_OSAL_printf(const char* fmt, ...);
    void SF_OSAL_LogWrite(OSAL_LogPriority_e priority, uint32_t component, const char* fmt, ...);
    void SF_OSAL_LogSetFilter(OSAL_LogPriority_e priorityLevel, uint32_t componentMatch);
    int getch(void);
    int kbhit(void);
    int putch(int ch);
    int getline(char* buffer, int buflen);

    int GPS_getch(void);
    int GPS_kbhit(void);
    int GPS_putch(int ch);
    int GPS_putBlock(const void*, size_t);
    int GPS_readBlock(void*, size_t);
    int GPS_getline(char* buffer, size_t buflen);
#ifdef __cplusplus
}
#endif

#endif