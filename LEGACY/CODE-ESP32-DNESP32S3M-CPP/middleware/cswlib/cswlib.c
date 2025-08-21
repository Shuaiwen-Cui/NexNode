#include <stdio.h>
#include "cswlib.h"
#include "cswtest.hpp"  // 引入 C++ 函数接口

void cswlib_init()
{
    printf("Initializing cswlib...\n");
    run_csw_test();  // 调用 C++ 的函数
}
