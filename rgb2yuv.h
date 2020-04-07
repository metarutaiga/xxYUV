//==============================================================================
// xxYUV : rgb2yuv Header
//
// Copyright (c) 2020 TAiGA
// https://github.com/metarutaiga/xxYUV
//==============================================================================

//------------------------------------------------------------------------------
template<int rgbWidth, bool interleaved, bool firstV>
void rgb2yuv(int width, int height, const void* rgb, int strideRGB, void* y, void* u, void* v, int strideY, int strideU, int strideV);
//------------------------------------------------------------------------------
void rgb2yuv_yu12(int width, int height, const void* rgb, void* yuv, int rgbWidth = 3, int strideRGB = 0, int alignWidth = 16, int alignHeight = 16);
void rgb2yuv_yv12(int width, int height, const void* rgb, void* yuv, int rgbWidth = 3, int strideRGB = 0, int alignWidth = 16, int alignHeight = 16);
void rgb2yuv_nv12(int width, int height, const void* rgb, void* yuv, int rgbWidth = 3, int strideRGB = 0, int alignWidth = 16, int alignHeight = 16);
void rgb2yuv_nv21(int width, int height, const void* rgb, void* yuv, int rgbWidth = 3, int strideRGB = 0, int alignWidth = 16, int alignHeight = 16);
//------------------------------------------------------------------------------
