//==============================================================================
// xxYUV : rgb2yuv Header
//
// Copyright (c) 2020 TAiGA
// https://github.com/metarutaiga/xxYUV
//==============================================================================

//------------------------------------------------------------------------------
template<int rgbWidth, bool rgbSwizzle, bool interleaved, bool firstU, bool fullRange>
void rgb2yuv(int width, int height, const void* rgb, int strideRGB, void* y, void* u, void* v, int strideY, int strideU, int strideV);
//------------------------------------------------------------------------------
void rgb2yuv_yu12(int width, int height, const void* rgb, void* yuv, int rgbWidth = 3, bool rgbSwizzle = false, bool fullRange = true, int strideRGB = 0, int alignWidth = 16, int alignHeight = 1, int alignSize = 1);
void rgb2yuv_yv12(int width, int height, const void* rgb, void* yuv, int rgbWidth = 3, bool rgbSwizzle = false, bool fullRange = true, int strideRGB = 0, int alignWidth = 16, int alignHeight = 1, int alignSize = 1);
void rgb2yuv_nv12(int width, int height, const void* rgb, void* yuv, int rgbWidth = 3, bool rgbSwizzle = false, bool fullRange = true, int strideRGB = 0, int alignWidth = 16, int alignHeight = 1, int alignSize = 1);
void rgb2yuv_nv21(int width, int height, const void* rgb, void* yuv, int rgbWidth = 3, bool rgbSwizzle = false, bool fullRange = true, int strideRGB = 0, int alignWidth = 16, int alignHeight = 1, int alignSize = 1);
//------------------------------------------------------------------------------
