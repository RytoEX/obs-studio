#pragma once

#include "../c99defs.h"
/* Do not try to define WIN32_LEAN_AND_MEAN here. Code in window-helpers.c requires it for malloc.
 * Alternatively, we could include <stdlib.h> and <malloc.h> instead.
 */
#include <Windows.h>

#ifdef __cplusplus
extern "C" {
#endif

/* this is a workaround to A/Vs going crazy whenever certain functions (such as
 * OpenProcess) are used */
void *ms_get_obfuscated_func(HMODULE module, const char *str, uint64_t val);

#ifdef __cplusplus
}
#endif
