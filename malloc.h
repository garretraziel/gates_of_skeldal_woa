#pragma once

#if defined(__APPLE__)
#include <stdlib.h>
#else
#include_next <malloc.h>
#endif
