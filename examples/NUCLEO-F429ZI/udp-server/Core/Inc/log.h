#ifndef LOG_H
#define LOG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cmsis_os.h"
#include <stdio.h>
#include <stdbool.h>

extern osMutexId_t printf_mutex_id;

#if 1
#define PRINTF_MUTEX_LOCK()	osMutexAcquire(printf_mutex_id, osWaitForever)
#define PRINTF_MUTEX_UNLOCK()	osMutexRelease(printf_mutex_id)
#else
#define PRINTF_MUTEX_LOCK()
#define PRINTF_MUTEX_UNLOCK()
#endif

#ifdef USE_LOG
#define LOG(...) do {\
	PRINTF_MUTEX_LOCK();\
	printf("[%s:%d] ",__func__, __LINE__);\
	printf(__VA_ARGS__);\
	puts("\r\n");\
	PRINTF_MUTEX_UNLOCK();\
} while(0)
#else
#define LOG(...)
#endif

void init_log(void);

#ifdef __cplusplus
}
#endif

#endif
