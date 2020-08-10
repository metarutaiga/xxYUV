//==============================================================================
// xxYUV : yuv2rgb Header
//
// Copyright (c) 2020 TAiGA
// https://github.com/metarutaiga/xxYUV
//==============================================================================

#ifndef xxYUV_EXPORT
#define xxYUV_EXPORT
#endif

//------------------------------------------------------------------------------
template<int rgbWidth, bool rgbSwizzle, bool interleaved, bool firstU, bool fullRange>
xxYUV_EXPORT void yuv2rgb(int width, int height, const void* y, const void* u, const void* v, int strideY, int strideU, int strideV, void* rgb, int strideRGB);
//------------------------------------------------------------------------------
xxYUV_EXPORT void yuv2rgb_yu12(int width, int height, const void* yuv, void* rgb, bool fullRange = true, int rgbWidth = 3, bool rgbSwizzle = false, int strideRGB = 0, int alignWidth = 16, int alignHeight = 1, int alignSize = 1);
xxYUV_EXPORT void yuv2rgb_yv12(int width, int height, const void* yuv, void* rgb, bool fullRange = true, int rgbWidth = 3, bool rgbSwizzle = false, int strideRGB = 0, int alignWidth = 16, int alignHeight = 1, int alignSize = 1);
xxYUV_EXPORT void yuv2rgb_nv12(int width, int height, const void* yuv, void* rgb, bool fullRange = true, int rgbWidth = 3, bool rgbSwizzle = false, int strideRGB = 0, int alignWidth = 16, int alignHeight = 1, int alignSize = 1);
xxYUV_EXPORT void yuv2rgb_nv21(int width, int height, const void* yuv, void* rgb, bool fullRange = true, int rgbWidth = 3, bool rgbSwizzle = false, int strideRGB = 0, int alignWidth = 16, int alignHeight = 1, int alignSize = 1);
//------------------------------------------------------------------------------
