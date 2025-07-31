/* X3F_PLATFORM.H
 *
 * Platform detection and iOS-specific definitions for x3f_extract
 *
 * Copyright 2015 - Roland and Erik Karlsson
 * BSD-style - see doc/copyright.txt
 *
 */

#ifndef X3F_PLATFORM_H
#define X3F_PLATFORM_H

#ifdef __APPLE__
  #include <TargetConditionals.h>
  #if TARGET_OS_IOS
    #define PLATFORM_IOS 1
    #define PLATFORM_MOBILE 1
  #endif
#endif

#ifdef PLATFORM_IOS
  #include <Foundation/Foundation.h>
  #include <UIKit/UIKit.h>
  
  /* iOS-specific file system functions */
  const char* x3f_get_documents_directory(void);
  const char* x3f_get_temp_directory(void);
  
  /* iOS memory management */
  void x3f_ios_memory_warning_handler(void);
  void x3f_ios_enter_background(void);
  void x3f_ios_enter_foreground(void);
  
  /* iOS threading using GCD instead of raw pthreads */
  #include <dispatch/dispatch.h>
  
  /* Use Metal instead of OpenCL on iOS */
  #ifdef __OBJC__
    #import <Metal/Metal.h>
    #import <MetalKit/MetalKit.h>
  #endif
  
  /* iOS doesn't support OpenCL, always disable */
  #undef USE_OPENCL
  #define USE_OPENCL 0
  
#else
  /* Desktop platforms can use OpenCL */
  #ifndef USE_OPENCL
    #define USE_OPENCL 1
  #endif
#endif

/* Platform-specific path separators */
#if defined(_WIN32) || defined(_WIN64)
  #define PATHSEP "\\"
  #define PATHSEPS "\\/:\"
#else
  #define PATHSEP "/"
  #ifdef PLATFORM_IOS
    #define PATHSEPS "/"
  #else
    #define PATHSEPS "/"
  #endif
#endif

/* Memory constraints for mobile platforms */
#ifdef PLATFORM_MOBILE
  #define X3F_MAX_MEMORY_MB 512
  #define X3F_ENABLE_STREAMING 1
#else
  #define X3F_MAX_MEMORY_MB 2048
  #define X3F_ENABLE_STREAMING 0
#endif

#endif /* X3F_PLATFORM_H */
