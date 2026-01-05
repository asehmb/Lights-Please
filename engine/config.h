// Central project configuration header.
// Include this from headers that need to respect project-wide settings.
#pragma once

// Project-level way to disable logging across the codebase:
// - Define LIGHTS_PLEASE_DISABLE_LOGS (e.g., via compiler flags) to force
//   NDEBUG to be defined and make logging macros no-ops.
// - Define LIGHTS_PLEASE_FORCE_DEBUG to ensure logging is enabled (undef NDEBUG).

#if defined(LIGHTS_PLEASE_DISABLE_LOGS)
#  ifndef NDEBUG
#    define NDEBUG
#  endif
#endif

#if defined(LIGHTS_PLEASE_FORCE_DEBUG)
#  ifdef NDEBUG
#    undef NDEBUG
#  endif
#endif

// Optionally expose a convenience macro to test whether logging is enabled.
#if defined(NDEBUG)
#  define LIGHTS_PLEASE_LOG_ENABLED 0
#else
#  define LIGHTS_PLEASE_LOG_ENABLED 1
#endif
