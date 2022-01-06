#ifndef RTC_H
#define RTC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"
#include <stdint.h>

HAL_StatusTypeDef Rtc_Get_DateTime(RTC_HandleTypeDef *hrtc, RTC_TimeTypeDef *time, RTC_DateTypeDef *date);
HAL_StatusTypeDef Rtc_Set_DateTime(RTC_HandleTypeDef *hrtc,
        uint8_t year, uint8_t month, uint8_t day,
        uint8_t hour, uint8_t min, uint8_t sec,
        uint8_t dow);
int RtcConvertMonthFromBSD2Bin(int bsd);
int RtcConvertMonthFromBin2BSD(int bin);

#ifdef __cplusplus
}
#endif

#endif
