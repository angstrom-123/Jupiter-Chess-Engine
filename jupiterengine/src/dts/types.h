#pragma once

#include <cstdint>

using ThreadId = uint8_t;
const ThreadId THREAD_ID_MAX = UINT8_MAX;
const ThreadId MANAGER_THREAD_ID = THREAD_ID_MAX - 1;
