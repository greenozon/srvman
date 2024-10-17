#pragma once
#include "../bzs_string.h"
#define BAZISLIB_NO_STRUCTURED_TIME

#define gettimeofday(a,b) microtime(a)
#include "../Posix/datetime.h"