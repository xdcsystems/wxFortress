#pragma once

#ifdef _WIN32

#include <stdlib.h>
#include <crtdbg.h>

#define TEST_MEMORY                                                                            \
  struct MemoryLeaksDetect                                                                     \
  {                                                                                            \
    MemoryLeaksDetect()                                                                        \
    {                                                                                          \
      _CrtMemCheckpoint(&s1);                                                                  \
    }                                                                                          \
    ~MemoryLeaksDetect()                                                                       \
    {                                                                                          \
      _CrtMemCheckpoint(&s2);                                                                  \
      if (_CrtMemDifference(&s3, &s1, &s2))                                                    \
      {                                                                                        \
        _CrtMemDumpStatistics(&s3);                                                            \
        EXPECT_FALSE("Memory leak detected! (Note: failed test case can cause memory leaks)"); \
      }                                                                                        \
    }                                                                                          \
    _CrtMemState s1, s2, s3;                                                                   \
  } memoryLeaksDetector;

#define TEST_MEMORY_AFTER(CODE) \
  struct MemoryLeaksDetectAfter \
  {                             \
    ~MemoryLeaksDetectAfter()   \
    {                           \
      CODE;                     \
    }                           \
    TEST_MEMORY                 \
  } memoryLeaksDetectorAfter;

#else

#define TEST_MEMORY
#define TEST_MEMORY_AFTER(CODE)

#endif
