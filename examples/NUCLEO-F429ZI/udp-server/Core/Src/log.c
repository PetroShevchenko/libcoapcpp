#include "log.h"

osMutexId_t printf_mutex_id;

const osMutexAttr_t printf_mutex_attr = {
  "printf_mutex",      // human readable mutex name
  osMutexPrioInherit
  | osMutexRobust
  | osMutexRecursive,  // attr_bits
  NULL,                // memory for control block
  0U                   // size for control block
};

void init_log(void)
{
	printf_mutex_id = osMutexNew (&printf_mutex_attr);
}
