#pragma once

#if defined(FLOAT_WEIGHTS)

#include "float.h"
typedef double weight_t;
#define WEIGHT_MAX DBL_MAX
#define WGT_FMT "f"
#else

#include "stdint.h"
#include "inttypes.h"
typedef uint64_t weight_t;
#define WEIGHT_MAX INT64_MAX
#define WGT_FMT PRId64
#endif