#ifndef _COMMON_H_
#define _COMMON_H_
#include <Arduino.h>
#include <functional>
#include "serial.h"
#define _TASK_STD_FUNCTION
#include <TaskScheduler.h>

#define STR_EQUALS(a, b) strcmp(a, b) == 0
#define STR_CASE_EQUALS(a, b) strcasecmp(a, b) == 0
#define STR_N_EQUALS(a, b) strncmp(a, b, strlen(b)) == 0

Scheduler *runner;

#endif