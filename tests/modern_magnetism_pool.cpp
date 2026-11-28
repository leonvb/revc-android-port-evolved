// Exercise the actual generation-checked pool handles used by magnetism.
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
typedef int32_t int32;
typedef uint32_t uint32;
typedef uint8_t uint8;
#define nil nullptr
#define FIX_BUGS
#define debug(...) ((void)0)
#include "../src/core/templates.h"
#include "../src/core/ModernAimMagnetism.h"
struct TestPed { int value; };
int main()
{
	CPool<TestPed> pool(1, "test");
	TestPed *first = pool.New();
	int firstHandle = pool.GetIndex(first);
	ModernAimMagnetism::State state; state.Reset(); state.Acquire(firstHandle);
	assert(pool.GetAt(state.target) == first);
	pool.Delete(first);
	assert(pool.GetAt(state.target) == nullptr);
	assert(!state.Visible(false, false, 0.016f) && state.lockedOut);
	TestPed *replacement = pool.New();
	int newHandle = pool.GetIndex(replacement);
	assert(newHandle != firstHandle && pool.GetAt(firstHandle) == nullptr);
	assert(pool.GetAt(newHandle) == replacement);
	assert(!state.CanAcquire());
	state.Reset();
	assert(state.CanAcquire());
}
