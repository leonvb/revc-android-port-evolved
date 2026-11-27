#include "../vendor/librw/src/gl/gl3fbomates.h"
#include <cassert>

struct Raster {
	Raster *mate;
	Raster *attached;
	int detaches;
	int attaches;
	Raster() : mate(0), attached(0), detaches(0), attaches(0) {}
};
struct Ops {
	Raster *&mate(Raster *r) { return r->mate; }
	void detach(Raster *c) { c->attached = 0; ++c->detaches; }
	void attach(Raster *c, Raster *d) { c->attached = d; ++c->attaches; }
};
int main()
{
	Raster a, b, x, y;
	Ops ops;
	using rw::gl3::rebindFramebufferDepth;
	// A color-only framebuffer requires no attachment calls.
	rebindFramebufferDepth(&a, static_cast<Raster *>(0), ops);
	assert(!a.mate && !a.attached && a.detaches == 0);
	rebindFramebufferDepth(&a, &x, ops);
	assert(a.mate == &x && x.mate == &a && a.attached == &x);
	rebindFramebufferDepth(&a, &x, ops);
	assert(a.attaches == 1 && a.detaches == 0);
	// Replacing a depth raster releases the old reciprocal link.
	rebindFramebufferDepth(&a, &y, ops);
	assert(!x.mate && a.mate == &y && y.mate == &a && a.attached == &y);
	// Moving a depth raster disconnects the previous color target.
	rebindFramebufferDepth(&b, &y, ops);
	assert(!a.mate && !a.attached && b.mate == &y && y.mate == &b && b.attached == &y);
	// Replacing both sides simultaneously must unlink both former mates.
	rebindFramebufferDepth(&a, &x, ops);
	rebindFramebufferDepth(&b, &x, ops);
	assert(!a.mate && !a.attached && !y.mate);
	assert(b.mate == &x && x.mate == &b && b.attached == &x);
	rebindFramebufferDepth(&b, static_cast<Raster *>(0), ops);
	assert(!b.mate && !x.mate && !b.attached);

	// Exercise long sequences of collisions, replacements, and removals.
	Raster colors[5], depths[4];
	unsigned state = 0x12345678u;
	for(int step = 0; step < 10000; ++step){
		state = state * 1664525u + 1013904223u;
		unsigned ci = state % 5;
		state = state * 1664525u + 1013904223u;
		unsigned di = state % 5;
		rebindFramebufferDepth(&colors[ci], di == 4 ? static_cast<Raster *>(0) : &depths[di], ops);
		for(int i = 0; i < 5; ++i){
			assert(colors[i].mate == colors[i].attached);
			if(colors[i].mate) assert(colors[i].mate->mate == &colors[i]);
		}
		for(int i = 0; i < 4; ++i)
			if(depths[i].mate) assert(depths[i].mate->mate == &depths[i]);
	}
}
