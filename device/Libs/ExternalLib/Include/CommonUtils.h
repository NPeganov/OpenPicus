#ifndef COMMON_UTILS_H
#define COMMON_UTILS_H

#include "taskFlyport.h"

void GetError();
BOOL ProcessCommand();
BOOL RunClock(const char * TimestampString);
void GetClockValue(const char * TimestampDest);

#endif


