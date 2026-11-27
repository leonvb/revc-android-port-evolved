#ifndef REVC_MODERN_POSTFX_POLICY_H
#define REVC_MODERN_POSTFX_POLICY_H

// Shader capture parameters for an image copied to the top-left of a
// power-of-two D3D texture, or to the upper portion of a GL texture. The
// shader accounts for the backend's different vertical texture orientation.
namespace ModernPostFXPolicy {
inline bool CaptureParams(int width, int height, int texWidth, int texHeight, float out[4])
{
	if (!out || width <= 0 || height <= 0 || texWidth < width || texHeight < height)
		return false;
	out[0] = 1.0f / texWidth;
	out[1] = 1.0f / texHeight;
	out[2] = float(width) / texWidth;
	out[3] = float(height) / texHeight;
	return true;
}
}
#endif
