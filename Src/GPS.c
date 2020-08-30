#include "GPS.h"
#include "main.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#define MAX_TRIALS 20

GPS_t GPS;
//##################################################################################################################
double convertDegMinToDecDeg (float degMin) {
    double min = 0.0;
    double decDeg = 0.0;

    // get the minutes, fmod() requires double
    min = fmod((double)degMin, 100.0);

    // rebuild coordinates in decimal degrees
    degMin = (int) (degMin/100);
    decDeg = degMin + (min/60);

    return decDeg;
}
//##################################################################################################################
void GPS_Init(void) {
    GPS.rxIndex = 0;
}
//##################################################################################################################
void GPS_CallBack(void) {
    GPS.LastTime=HAL_GetTick();
    if(GPS.rxIndex < sizeof(GPS.rxBuffer)-2) {
        GPS.rxBuffer[GPS.rxIndex] = GPS.rxTmp;
        GPS.rxIndex++;
    }
    HAL_UART_Receive_IT(&_GPS_USART,&GPS.rxTmp,1);
}

char *strtok_fr (char *s, char delim, char **save_ptr) {
    char *tail;
    char c;

    if (s == NULL) {
        s = *save_ptr;
    }
    tail = s;
    if ((c = *tail) == '\0') {
        s = NULL;
    }
    else {
        do {
            if (c == delim) {
                *tail++ = '\0';
                break;
           }
        }while ((c = *++tail) != '\0');
    }
    *save_ptr = tail;
    return s;
}

char *strtok_f (char *s, char delim) {
    static char *save_ptr;

    return strtok_fr (s, delim, &save_ptr);
}

//##################################################################################################################
void GPS_Process(void) {
    if( (HAL_GetTick() - GPS.LastTime > 50) && (GPS.rxIndex>0)) {
        char *str;
        #if (_GPS_DEBUG==1)
            debug_str(GPS.rxBuffer);
        #endif
        str=strstr((char*)GPS.rxBuffer,"$GNGGA,");
        if(str!=NULL) {
            memset(&GPS.GPGGA,0,sizeof(GPS.GPGGA));
            sscanf(str,"$GNGGA,%f,%f,%c,%f,%c,%d,%d,%f,%f,%c,%f,%c,%s,*%2s\r\n",
                &GPS.GPGGA.UTC_Time,
                &GPS.GPGGA.Latitude,
                &GPS.GPGGA.NS_Indicator,
                &GPS.GPGGA.Longitude,
                &GPS.GPGGA.EW_Indicator,
                &GPS.GPGGA.PositionFixIndicator,
                &GPS.GPGGA.SatellitesUsed,
                &GPS.GPGGA.HDOP,
                &GPS.GPGGA.MSL_Altitude,
                &GPS.GPGGA.MSL_Units,
                &GPS.GPGGA.Geoid_Separation,
                &GPS.GPGGA.Geoid_Units,
                GPS.GPGGA.DiffRefStationID,
                GPS.GPGGA.CheckSum);

            if(GPS.GPGGA.NS_Indicator == 0)
                GPS.GPGGA.NS_Indicator = '-';
            if(GPS.GPGGA.EW_Indicator == 0)
                GPS.GPGGA.EW_Indicator = '-';
            if(GPS.GPGGA.Geoid_Units == 0)
                GPS.GPGGA.Geoid_Units = '-';
            if(GPS.GPGGA.MSL_Units == 0)
                GPS.GPGGA.MSL_Units =' -';

            GPS.GPGGA.LatitudeDecimal = convertDegMinToDecDeg(GPS.GPGGA.Latitude);
            GPS.GPGGA.LongitudeDecimal = convertDegMinToDecDeg(GPS.GPGGA.Longitude);
        }

        memset(GPS.rxBuffer,0,sizeof(GPS.rxBuffer));
        GPS.rxIndex = 0;
    }
    HAL_UART_Receive_IT(&_GPS_USART,&GPS.rxTmp,1);
}

float GPS_Get_Lat(void) {
    return GPS.GPGGA.LatitudeDecimal;
}

float GPS_Get_Lon(void) {
    return GPS.GPGGA.LongitudeDecimal;
}

void GPS_Query(void) {
    for (int i = 0; i < MAX_TRIALS; i++) {
        GPS_Process();
        HAL_Delay(50);
        if (GPS_Get_Lat() != 0.0 && GPS_Get_Lon() != 0.0) {
            break;
        }
    }

}


//##################################################################################################################
