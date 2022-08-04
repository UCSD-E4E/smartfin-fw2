#include "conio.hpp"

#include "Particle.h"
#include "system.hpp"

#include <cstdio>

char SF_OSAL_printfBuffer[SF_OSAL_PRINTF_BUFLEN];
OSAL_LogPriority_e _priorityLevel = OSAL_LOG_PRIO_DEBUG;
uint32_t _componentMatch = 1;
extern "C"
{
    void SF_OSAL_LogWrite(OSAL_LogPriority_e priority, uint32_t component, const char* fmt, ...)
    {
        va_list vargs;
        if(priority > _priorityLevel)
        {
            return;
        }
        if(component != _componentMatch)
        {
            return;
        }
        va_start(vargs, fmt);
        vsnprintf(SF_OSAL_printfBuffer, SF_OSAL_PRINTF_BUFLEN, fmt, vargs);
        va_end(vargs);
        Serial.write(SF_OSAL_printfBuffer);
    }
    void SF_OSAL_LogSetFilter(OSAL_LogPriority_e priorityLevel, uint32_t componentMatch)
    {
        _priorityLevel = priorityLevel;
        _componentMatch = componentMatch;
    }

    int SF_OSAL_printf(const char* fmt, ...)
    {
        va_list vargs;
        int nBytes = 0;
        va_start(vargs, fmt);
        nBytes = vsnprintf(SF_OSAL_printfBuffer, SF_OSAL_PRINTF_BUFLEN, fmt, vargs);
        va_end(vargs);
        Serial.write(SF_OSAL_printfBuffer);
        return nBytes;
    }
    int GPS_putBlock(const void* pBuf, size_t bufLen)
    {
        return Serial5.write((uint8_t*)pBuf, bufLen);
    }

    int putch(int ch)
    {
        char outputBuf[2] = {(char)ch, 0};
        // Serial.print(outputBuf);
        Serial.write((char) ch);
        return ch;
    }
    int GPS_putch(int ch)
    {
        Serial5.write((char) ch);
        return ch;
    }

    int getch(void)
    {
        while(Serial.available() == 0)
        {
#ifdef SF_OSAL_GETCH_USE_WDOG
            // TODO: pet watchdog
#endif
            // os_thread_yield();
            delay(1);
        }
        return Serial.read();
    }

    int GPS_getch(void)
    {
        while(Serial5.available() == 0)
        {
#ifdef SF_OSAL_GETCH_USE_WDOG
            // TODO: pet watchdog
#endif
            os_thread_yield();
        }
        return Serial5.read();
    }
    int kbhit(void)
    {
        return Serial.available();
    }
    int GPS_kbhit(void)
    {
        return Serial5.available();
    }

    int getline(char* buffer, int buflen)
    {
        int i = 0;
        char userInput;

        while(i < buflen)
        {
            if(kbhit())
            {
                userInput = getch();
                switch(userInput)
                {
                    case '\b':
                        i--;
                        putch('\b');
                        putch(' ');
                        putch('\b');
                        break;
                    default:
                        buffer[i++] = userInput;
                        putch(userInput);
                        break;
                    case '\r':
                        buffer[i++] = 0;
                        putch('\r');
                        putch('\n');
                        return i;
                }
            }
            if(!pSystemDesc->flags->hasCharger) {
                return -1;
            }
            
        }
        return i;
    }

    int GPS_readBlock(void* pBuffer, size_t bufLen)
    {
        size_t i = 0;
        uint8_t* pByte = (uint8_t*) pBuffer;
        for(i = 0; i < bufLen; i++)
        {
            pByte[i] = GPS_getch();
        }
        return i;
    }
    int GPS_getline(char* buffer, size_t buflen)
    {
        size_t i = 0;
        char userInput;

        while(i < buflen)
        {
            if(GPS_kbhit())
            {
                userInput = GPS_getch();
                switch(userInput)
                {
                    case '\b':
                        i--;
#ifdef SF_OSAL_GPS_GETLINE_ECHO
                        GPS_putch('\b');
                        GPS_putch(' ');
                        GPS_putch('\b');
#endif
                        break;
                    default:
                        buffer[i++] = userInput;
#ifdef SF_OSAL_GPS_GETLINE_ECHO
                        GPS_putch(userInput);
#endif
                        break;
                    case '\r':
                        buffer[i++] = 0;
#ifdef SF_OSAL_GPS_GETLINE_ECHO
                        GPS_putch('\r');
                        GPS_putch('\n');
#endif
                        return i;
                }
            }
            
        }
        return i;
    }

}