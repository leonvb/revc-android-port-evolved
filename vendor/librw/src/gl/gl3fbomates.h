#ifndef RW_GL3_FBOMATES_H
#define RW_GL3_FBOMATES_H

// Maintain a one-to-one relationship between a color raster and a depth
// raster. Ops supplies mate(raster), detach(color), and attach(color, depth).
// Kept independent of GL so reassignment and removal can be tested without
// a context. A null depth represents a color-only target.
namespace rw { namespace gl3 {
template <class RasterType, class Ops>
void rebindFramebufferDepth(RasterType *color, RasterType *depth, Ops &ops)
{
	RasterType *oldDepth = ops.mate(color);
	if(oldDepth == depth)
		return;
	if(oldDepth){
		ops.detach(color);
		ops.mate(oldDepth) = 0;
		ops.mate(color) = 0;
	}
	if(depth){
		RasterType *oldColor = ops.mate(depth);
		if(oldColor){
			ops.detach(oldColor);
			ops.mate(oldColor) = 0;
			ops.mate(depth) = 0;
		}
		ops.attach(color, depth);
		ops.mate(color) = depth;
		ops.mate(depth) = color;
	}
}
} }
#endif
