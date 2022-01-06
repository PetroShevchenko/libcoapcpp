#include "rtc.h"
#include "log.h"
#include <time.h>
#include <string.h>


HAL_StatusTypeDef Rtc_Get_DateTime(RTC_HandleTypeDef *hrtc, RTC_TimeTypeDef *time, RTC_DateTypeDef *date)
{
	HAL_StatusTypeDef res;

	res = HAL_RTC_GetTime(hrtc, time, RTC_FORMAT_BIN);
	if(res != HAL_OK) {
		LOG("HAL_RTC_GetTime failed: %d\r\n", res);
		return res;
	}

	res = HAL_RTC_GetDate(hrtc, date, RTC_FORMAT_BIN);
	if(res != HAL_OK) {
		LOG("HAL_RTC_GetDate failed: %d\r\n", res);
	}
	return res;
}

HAL_StatusTypeDef Rtc_Set_DateTime(RTC_HandleTypeDef *hrtc,
        uint8_t year, uint8_t month, uint8_t day,
        uint8_t hour, uint8_t min, uint8_t sec,
        uint8_t dow)
{
    HAL_StatusTypeDef res;
    RTC_TimeTypeDef time;
    RTC_DateTypeDef date;

    memset(&time, 0, sizeof(time));
    memset(&date, 0, sizeof(date));

    date.WeekDay = dow;
    date.Year = year;
    date.Month = month;
    date.Date = day;

    res = HAL_RTC_SetDate(hrtc, &date, RTC_FORMAT_BIN);
    if(res != HAL_OK) {
        LOG("HAL_RTC_SetDate failed: %d\r\n", res);
        return res;
    }

    time.Hours = hour;
    time.Minutes = min;
    time.Seconds = sec;

    res = HAL_RTC_SetTime(hrtc, &time, RTC_FORMAT_BIN);
    if(res != HAL_OK) {
        LOG("HAL_RTC_SetTime failed: %d\r\n", res);
    }

    return res;
}

int RtcConvertMonthFromBSD2Bin(int bsd)
{
	if (bsd < 10)
	{
		return bsd;
	}
	else if (bsd < 0x13)
	{
		return (bsd & 0x3) + 10;
	}
	return -1;
}

int RtcConvertMonthFromBin2BSD(int bin)
{
	if (bin < 10)
	{
		return bin;
	}
	else if (bin < 13)
	{
		return (bin & 0x7) + 14;
	}
	return -1;
}
