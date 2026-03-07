#pragma once

// ============================================================================
// WINDOWS CONFLICT FIX
// ============================================================================
// This header MUST be included BEFORE any Windows headers or standard headers
// to prevent macro conflicts with C++ code.
//
// Windows.h defines macros like CRITICAL and constant that conflict with:
// - enum class LogLevel { CRITICAL }
// - CUDA/OpenCL __constant__ keyword
// ============================================================================

#ifdef _WIN32
    // Prevent Windows.h from defining problematic macros
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #ifndef NOGDI
        #define NOGDI
    #endif
    #ifndef NOSERVICE
        #define NOSERVICE
    #endif
    #ifndef NOIME
        #define NOIME
    #endif
    #ifndef NOMCX
        #define NOMCX
    #endif
    #ifndef NOHELP
        #define NOHELP
    #endif

    // Undefine macros that conflict with C++ code
    // These are defined by Windows.h or other Windows headers
    #ifdef CRITICAL
        #undef CRITICAL
    #endif
    #ifdef constant
        #undef constant
    #endif
    #ifdef ERROR
        #undef ERROR
    #endif
    #ifdef IGNORE
        #undef IGNORE
    #endif
    #ifdef DELETE
        #undef DELETE
    #endif
    #ifdef min
        #undef min
    #endif
    #ifdef max
        #undef max
    #endif
#endif
