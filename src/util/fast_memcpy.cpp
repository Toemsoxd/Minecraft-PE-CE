#include <cstring>
#include <cstddef>

/*
 * Windows CE / ARMV4 fallback.
 *
 * The PSP implementation used MIPS/VFPU instructions.  The portable
 * renderer does not require those instructions, so keep the same symbol
 * and let the CE C runtime provide the copy.  A later ARMV4-specific
 * implementation can optimize this without changing callers.
 */
void* memcpy_vfpu(void* dst, const void* src, std::size_t bytes)
{
    return std::memcpy(dst, src, bytes);
}
