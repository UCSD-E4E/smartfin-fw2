#ifndef __CONIO_HPP__
#define __CONIO_HPP__
#include <stdarg.h>
#include <stddef.h>
#define SF_OSAL_PRINTF_BUFLEN   1024
#define SF_OSAL_GETCH_USE_WDOG
#define SF_OSAL_GPS_GETLINE_ECHO
#ifdef __cplusplus
extern "C"
{
#endif
    int SF_OSAL_printf(const char* fmt, ...);
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