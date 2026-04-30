#pragma once

#if defined(__APPLE__)
#include <stdlib.h>
#elif defined(_MSC_VER)
// MSVC doesn't support #include_next; this wrapper shadows the real malloc.h.
// Declare _alloca directly (it's a compiler built-in on MSVC).
#include <stdlib.h>
#ifdef __cplusplus
extern "C"
#endif
void *_alloca(size_t);
#ifndef alloca
#define alloca _alloca
#endif
#else
#include_next <malloc.h>
#endif
