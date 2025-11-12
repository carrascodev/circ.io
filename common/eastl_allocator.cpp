#include <EASTL/internal/config.h>
#include <new>
#include <cstdlib>

// EASTL requires you to define your own operator new/delete overloads
// These are used for EASTL containers

void* operator new[](size_t size, const char* /*name*/, int /*flags*/,
    unsigned /*debugFlags*/, const char* /*file*/, int /*line*/)
{
    return malloc(size);
}

void* operator new[](size_t size, size_t /*alignment*/, size_t /*alignmentOffset*/,
    const char* /*name*/, int /*flags*/, unsigned /*debugFlags*/, const char* /*file*/, int /*line*/)
{
    // For simplicity, ignoring alignment for now
    return malloc(size);
}
