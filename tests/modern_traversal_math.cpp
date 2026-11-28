#include "../src/peds/ModernTraversalMath.h"
#include <cassert>
#include <initializer_list>
using namespace ModernTraversalMath;
int main()
{
    for (int fps : {30, 60, 120}) {
        Pivot pivot; pivot.Reset();
        float dt = 1.0f / fps;
        assert(pivot.Update(1, 0, 1, dt) == 1);
        assert(pivot.Update(-1, 0, 1, dt) < 0.4f);
        float elapsed = dt;
        float previous = 0.35f;
        while (elapsed < 0.12f) {
            float factor = pivot.Update(-1, 0, 1, dt);
            assert(factor >= previous && factor <= 1.0f);
            previous = factor;
            elapsed += dt;
        }
        assert(pivot.Update(-1, 0, 1, dt) == 1.0f);
        pivot.Reset();
        assert(pivot.Update(1, 0, 0.1f, dt) == 1.0f);
        assert(pivot.Update(-1, 0, 0.1f, dt) == 1.0f); // low-speed turn
        assert(pivot.Update(0, 0, 1, dt) == 1.0f);
        assert(pivot.Update(1, 0, 1, dt) == 1.0f); // neutral resets direction
        assert(pivot.Update(0, 1, 1, dt) == 1.0f); // right angle is not a reversal
    }
}
