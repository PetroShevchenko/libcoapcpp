/**
 ******************************************************************************
 * @file      syscalls.c
 * @author    Auto-generated by STM32CubeIDE
 * @brief     STM32CubeIDE Minimal System calls file
 *
 *            For more information about which c-functions
 *            need which of these lowlevel functions
 *            please consult the Newlib libc-manual
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under BSD 3-Clause license,
 * the "License"; You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                        opensource.org/licenses/BSD-3-Clause
 *
 ******************************************************************************
 */

/* Includes */
#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <sys/times.h>
#include "rtc.h"

/* Variables */
extern int __io_putchar(int ch) __attribute__((weak));
extern int __io_getchar(void) __attribute__((weak));
extern RTC_HandleTypeDef hrtc;

char *__env[1] = { 0 };
char **environ = __env;


/* Functions */
void initialise_monitor_handles()
{
}

int _getpid(void)
{
	return 1;
}

int _kill(int pid, int sig)
{
	errno = EINVAL;
	return -1;
}

void _exit (int status)
{
	_kill(status, -1);
	while (1) {}		/* Make sure we hang here */
}

__attribute__((weak)) int _read(int file, char *ptr, int len)
{
	int DataIdx;

	for (DataIdx = 0; DataIdx < len; DataIdx++)
	{
		*ptr++ = __io_getchar();
	}

return len;
}

__attribute__((weak)) int _write(int file, char *ptr, int len)
{
	int DataIdx;

	for (DataIdx = 0; DataIdx < len; DataIdx++)
	{
		__io_putchar(*ptr++);
	}
	return len;
}

int _close(int file)
{
	return -1;
}


int _fstat(int file, struct stat *st)
{
	st->st_mode = S_IFCHR;
	return 0;
}

int _isatty(int file)
{
	return 1;
}

int _lseek(int file, int ptr, int dir)
{
	return 0;
}

int _open(char *path, int flags, ...)
{
	/* Pretend like we always fail */
	return -1;
}

int _wait(int *status)
{
	errno = ECHILD;
	return -1;
}

int _unlink(char *name)
{
	errno = ENOENT;
	return -1;
}

int _times(struct tms *buf)
{
	return -1;
}

int _stat(char *file, struct stat *st)
{
	st->st_mode = S_IFCHR;
	return 0;
}

int _link(char *old, char *new)
{
	errno = EMLINK;
	return -1;
}

int _fork(void)
{
	errno = EAGAIN;
	return -1;
}

int _execve(char *name, char **argv, char **env)
{
	errno = ENOMEM;
	return -1;
}

int _settimeofday (const struct timeval * tp, const struct timezone * tzvp)
{
	struct tm *tmp;
	if (tp) {
		tmp = localtime(&tp->tv_sec);
		if (tmp == NULL)
		{
			return -1;
		}
		uint8_t year = tmp->tm_year - 100;
		int month = RtcConvertMonthFromBin2BSD(tmp->tm_mon);
		month = month > 0 ? month : 1;
		uint8_t dow = tmp->tm_wday > 0 ? tmp->tm_wday : 0x7;

		if (Rtc_Set_DateTime(&hrtc, year, month,tmp->tm_mday,
				tmp->tm_hour,tmp->tm_min,
				tmp->tm_sec, dow) != HAL_OK)
		{
			errno = EPERM;
			return -1;
		}
	}
	else {
		errno = EINVAL;
		return -1;
	}

  return 0;
}

int _gettimeofday (struct timeval * tp, void * tzvp)
{
  struct timezone *tzp = tzvp;
  struct tm t;
  RTC_TimeTypeDef time;
  RTC_DateTypeDef date;

  if (tp) {
	  if( Rtc_Get_DateTime(&hrtc, &time, &date) != HAL_OK)
	  {
		  return -1;
	  }
	  t.tm_sec = time.Seconds;
	  t.tm_min = time.Minutes;
	  t.tm_hour = time.Hours;
	  t.tm_mday = date.Date;
	  int month = RtcConvertMonthFromBSD2Bin(date.Month);
	  t.tm_mon = month > 0 ? month : 1;
	  t.tm_year = date.Year + 100;
	  t.tm_wday = date.WeekDay < 0x7 ? date.WeekDay : 0;

	  tp->tv_sec = mktime(&t);
      tp->tv_usec = 0;
  }

  if (tzp) {
      tzp->tz_minuteswest = 0;
      tzp->tz_dsttime = 0;
  }

  return 0;
}

int stime ( const time_t *when)
{
  struct timeval tv;

  if (when == NULL)
    {
      errno = EINVAL;
      return -1;
    }

  tv.tv_sec = *when;
  tv.tv_usec = 0;

  return _settimeofday (&tv, (struct timezone *) 0);
}
