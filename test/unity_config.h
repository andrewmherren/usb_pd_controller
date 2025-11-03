#ifndef UNITY_CONFIG_H
#define UNITY_CONFIG_H

// Required standard headers for Unity config
#include <stdio.h>
#include <stdlib.h>

// Unity configuration for comprehensive testing with CI/CD support
// Only define these if not already defined to avoid redefinition warnings

#ifndef UNITY_INCLUDE_PRINT_FORMATTED
#define UNITY_INCLUDE_PRINT_FORMATTED
#endif

#ifndef UNITY_INCLUDE_DOUBLE
#define UNITY_INCLUDE_DOUBLE
#endif

#ifndef UNITY_SUPPORT_64
#define UNITY_SUPPORT_64
#endif

#ifndef UNITY_INCLUDE_CONFIG_H
#define UNITY_INCLUDE_CONFIG_H
#endif

// Memory leak detection for native platform
#ifdef NATIVE_PLATFORM
#define UNITY_INCLUDE_EXEC_TIME
#define UNITY_OUTPUT_CHAR(c) putchar(c)
#define UNITY_OUTPUT_FLUSH() fflush(stdout)
#define UNITY_OUTPUT_START()
#define UNITY_OUTPUT_COMPLETE()
#endif

// ESP32 specific configurations
#ifdef ESP_PLATFORM
#define UNITY_OUTPUT_CHAR(c) Serial.write(c)
#define UNITY_OUTPUT_FLUSH() Serial.flush()
#define UNITY_OUTPUT_START() Serial.begin(115200)
#define UNITY_OUTPUT_COMPLETE() Serial.flush()
#endif

// Test result formatting for CI/CD parsing
#define UNITY_TEST_RESULT_START() printf("\n--- TEST RESULT START ---\n")
#define UNITY_TEST_RESULT_END() printf("--- TEST RESULT END ---\n")

// Memory usage reporting
#ifdef ESP_PLATFORM
#include <esp_system.h>
#define UNITY_PRINT_MEM()                                                      \
  printf("Free heap: %d bytes\n", esp_get_free_heap_size())
#else
#define UNITY_PRINT_MEM()
#endif

// Coverage and profiling support
#ifdef __COVERAGE__
#define UNITY_ENABLE_COVERAGE 1
extern void __gcov_flush(void);
#define UNITY_END_COVERAGE() __gcov_flush()
#else
#define UNITY_END_COVERAGE()
#endif

// ArduinoFake compatibility
#ifdef ARDUINOFAKE_ENABLE_STRING
#define UNITY_SUPPORT_STRING_COMPARISON
#endif

// Enhanced debugging for native platform
#ifdef NATIVE_PLATFORM
#define UNITY_PRINT_DEBUG_INFO()                                               \
  printf("[DEBUG] Unity running in native mode\n")
#else
#define UNITY_PRINT_DEBUG_INFO()
#endif

// SonarCloud compatible output formatting
#define UNITY_SONAR_OUTPUT

#endif // UNITY_CONFIG_H