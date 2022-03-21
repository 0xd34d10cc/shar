#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wall"
#pragma clang diagnostic ignored "-Wextra"
#pragma clang diagnostic ignored "-Weverything"
#endif

#if defined(_MSC_VER) && !defined(__INTEL_COMPILER)
#pragma warning( push, 0 )
#pragma warning(disable : 4101 4701 6285 6385 26495 26812)
#endif
