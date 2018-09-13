// Copyright 2018-2019 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <stdarg.h>
#include <fcntl.h>

int fcntl(int fd, int request, ...)
{
    int val, ret;
    va_list va;

    va_start(va, request);
    
    val = va_arg(va, int);
    ret = lwip_fcntl(fd, request, val);

    va_end(va);

    return ret;    
}
