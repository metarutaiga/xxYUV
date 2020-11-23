//==============================================================================
// xxYUV : yuv Header
//
// Copyright (c) 2020 TAiGA
// https://github.com/metarutaiga/xxYUV
//==============================================================================

#ifndef xxYUV_EXPORT
#define xxYUV_EXPORT
#endif

//------------------------------------------------------------------------------
template<bool interleaved, bool firstU, int iY, int iU, int iV, int iA>
xxYUV_EXPORT void yuv(int width, int height, const void* y, const void* u, const void* v, int strideY, int strideU, int strideV, void* output, int strideOutput);
//------------------------------------------------------------------------------
xxYUV_EXPORT void yuv_yu12_to_yuva(int width, int height, const void* input, void* output, bool yuvSwizzle = false, int strideOutput = 0, int alignWidth = 16, int alignHeight = 1, int alignSize = 1);
xxYUV_EXPORT void yuv_yv12_to_yuva(int width, int height, const void* input, void* output, bool yuvSwizzle = false, int strideOutput = 0, int alignWidth = 16, int alignHeight = 1, int alignSize = 1);
xxYUV_EXPORT void yuv_nv12_to_yuva(int width, int height, const void* input, void* output, bool yuvSwizzle = false, int strideOutput = 0, int alignWidth = 16, int alignHeight = 1, int alignSize = 1);
xxYUV_EXPORT void yuv_nv21_to_yuva(int width, int height, const void* input, void* output, bool yuvSwizzle = false, int strideOutput = 0, int alignWidth = 16, int alignHeight = 1, int alignSize = 1);
//------------------------------------------------------------------------------
