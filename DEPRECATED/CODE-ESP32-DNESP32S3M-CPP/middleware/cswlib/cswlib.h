#ifndef CSWLIB_H
#define CSWLIB_H

#ifdef __cplusplus
extern "C" {
#endif

void cswlib_init();  // C 接口

#ifdef __cplusplus
} // extern "C"
#include "cswtest.hpp"  // <-- 仅在 C++ 中包含类定义
#endif

#endif // CSWLIB_H
