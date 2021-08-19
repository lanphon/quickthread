cmake_minimum_required (VERSION 3.1)
cmake_policy(SET CMP0001 NEW)

project (QuickThread CXX C)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

message (STATUS "Checking OS and processor type.")

if (APPLE)
  list (LENGTH CMAKE_OSX_ARCHITECTURES N_OSX_ARCHITECTURES)
endif (APPLE)

if (NOT APPLE OR (APPLE AND (N_OSX_ARCHITECTURES LESS 2)))
  if (APPLE)
    if (N_OSX_ARCHITECTURES EQUAL 1)
      message (STATUS "The target architecture will be passed explicitly to the Apple compiler.")
      set (CMAKE_REQUIRED_FLAGS "-arch ${CMAKE_OSX_ARCHITECTURES}")
    endif (N_OSX_ARCHITECTURES EQUAL 1)
    unset (HAS__i386_DEFINED CACHE)
    unset (HAS__x86_64__DEFINED CACHE)
    unset (HAS__ppc__DEFINED CACHE)
  endif (APPLE)
  include (CheckSymbolExists)
  check_symbol_exists (__LP64__ "" HAS__LP64__DEFINED)
  check_symbol_exists (__sparc "" HAS__sparc_DEFINED)
  check_symbol_exists (__sparc__ "" HAS__sparc__DEFINED)
  check_symbol_exists (__hppa "" HAS__hppa_DEFINED)
  check_symbol_exists (__x86_64__ "" HAS__x86_64__DEFINED)
  check_symbol_exists (__i386 "" HAS__i386_DEFINED)
  check_symbol_exists (__aarch64__ "" HAS__aarch64__DEFINED)
  check_symbol_exists (__ppc__ "" HAS__ppc__DEFINED)
  check_symbol_exists (__powerpc "" HAS__powerpc_DEFINED)
endif (NOT APPLE OR (APPLE AND (N_OSX_ARCHITECTURES LESS 2)))




###############################################################################
# Configure QuickThreads
###############################################################################

# Set QuickThreads architecture based on the detected preprocessor symbols.
message (STATUS "Detect the target processor architecture for QuickThreads.")
if (ENABLE_PTHREADS)
  set (QT_ARCH "IGNORE") # Pthreads will be used for the SystemC coroutines.
elseif (MSVC)
  set (QT_ARCH "IGNORE") # Fibers will be used for the SystemC coroutines.
elseif (APPLE AND (N_OSX_ARCHITECTURES GREATER 1))
  set (QT_ARCH "IGNORE") # For universal libraries, Pthreads will be used for the SystemC coroutines.
elseif (HAS__sparc_DEFINED OR HAS__sparc__DEFINED)
  set (QT_ARCH "sparc")
elseif (HAS__hppa_DEFINED)
  set (QT_ARCH "hppa")
elseif (HAS__x86_64__DEFINED)
  set (QT_ARCH "x86_64")
elseif (HAS__i386_DEFINED)
  set (QT_ARCH "i386")
elseif (HAS__aarch64__DEFINED)
  set (QT_ARCH "aarch64")
elseif (HAS__ppc__DEFINED)
  set (QT_ARCH "powerpc_mach")
elseif (HAS__powerpc__DEFINED)
  set (QT_ARCH "powerpc_sys5")
else (ENABLE_PTHREADS)
  message (WARNING "QuickThreads is not supported on ${CMAKE_SYSTEM} on ${CMAKE_SYSTEM_PROCESSOR}.")
  set (QT_ARCH "IGNORE")
endif (ENABLE_PTHREADS)

if (QT_ARCH)
  # To build QuickThreads, enable the assembler support.
  enable_language (ASM)
else (QT_ARCH)
  # Otherwise, fall back to Pthreads or Fiber.
  if (NOT MSVC)
    set (ENABLE_PTHREADS TRUE CACHE BOOL
         "Use POSIX threads for SystemC processes instead of QuickThreads on Unix or Fiber on Windows."
         FORCE)
  endif (NOT MSVC)
endif (QT_ARCH)


###############################################################################
# Configure Pthreads or Fiber if necessary.
###############################################################################
if (WIN32 AND ENABLE_PTHREADS)
  message (FATAL_ERROR "Pthreads is not supported on ${CMAKE_SYSTEM}.")
endif (WIN32 AND ENABLE_PTHREADS)

if (ENABLE_PTHREADS OR (QT_ARCH AND NOT DISABLE_ASYNC_UPDATES) OR NOT QT_ARCH)
  set (THREADS_PREFER_PTHREAD_FLAG ON)
  find_package (Threads REQUIRED)
  if (ENABLE_PTHREADS AND NOT CMAKE_USE_PTHREADS_INIT)
    message (SEND_ERROR "Failed to find the Pthreads library required to implement the SystemC coroutines and async_request_update() of primitive channels on Unix.")
  endif (ENABLE_PTHREADS AND NOT CMAKE_USE_PTHREADS_INIT)
endif (ENABLE_PTHREADS OR (QT_ARCH AND NOT DISABLE_ASYNC_UPDATES) OR NOT QT_ARCH)

if (QT_ARCH STREQUAL "sparc")
  add_custom_command (OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/sysc/packages/qt/md/sparc.s
                      COMMAND sed 's/_qt_/qt_/' ${CMAKE_CURRENT_SOURCE_DIR}/sysc/packages/qt/md/sparc.s > ${CMAKE_CURRENT_BINARY_DIR}/sysc/packages/qt/md/sparc.s
                      DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/sysc/packages/qt/md/sparc.s)
endif (QT_ARCH STREQUAL "sparc")

if (APPLE)
  if (N_OSX_ARCHITECTURES EQUAL 1)
    if (QT_ARCH STREQUAL "i386")
      set (CMAKE_ASM_FLAGS "-arch i386")
    elseif (QT_ARCH STREQUAL "x86_64")
      set (CMAKE_ASM_FLAGS "-arch x86_64")
    elseif (QT_ARCH STREQUAL "powerpc_mach")
      set (CMAKE_ASM_FLAGS "-arch ppc")
    endif(QT_ARCH STREQUAL "i386")
  else (N_OSX_ARCHITECTURES EQUAL 1)
    set (CMAKE_ASM_FLAGS "")
  endif (N_OSX_ARCHITECTURES EQUAL 1)
endif (APPLE)

add_library (qt_lib STATIC
                     # QuickThreads
                     $<$<BOOL:${QT_ARCH}>:
                       qt/qt.c
                       $<$<STREQUAL:${QT_ARCH},sparc>:
                         ${CMAKE_CURRENT_BINARY_DIR}/qt/md/sparc.s>
                       $<$<STREQUAL:${QT_ARCH},hppa>:qt/md/hppa.s>
                       $<$<STREQUAL:${QT_ARCH},x86_64>:qt/md/iX86_64.s>
                       $<$<STREQUAL:${QT_ARCH},i386>:qt/md/i386.s>
                       $<$<STREQUAL:${QT_ARCH},aarch64>:qt/md/aarch64.s>
                       $<$<STREQUAL:${QT_ARCH},powerpc_mach>:
                         qt/md/powerpc_mach.s
                         qt/md/powerpc.c>
                       $<$<STREQUAL:${QT_ARCH},powerpc_sys5>:
                         qt/md/powerpc_sys5.s
                         qt/md/powerpc.c>
                       qt/md/aarch64.h
                       qt/md/axp.h
                       qt/md/hppa.h
                       qt/md/i386.h
                       qt/md/iX86_64.h
                       qt/md/ksr1.h
                       qt/md/m88k.h
                       qt/md/mips.h
                       qt/md/powerpc_mach.h
                       qt/md/powerpc_sys5.h
                       qt/md/sparc.h
                       qt/md/vax.h
                       qt/qt.h
                       qt/qtmd.h
                     >)

add_library (QT::qt_lib ALIAS qt_lib)
target_include_directories(qt_lib
    PUBLIC ${CMAKE_SOURCE_DIR}/qt)

add_subdirectory(samples)
