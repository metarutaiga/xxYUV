//==============================================================================
// xxYUV : yuv2rgb Header
//
// Copyright (c) 2020 TAiGA
// https://github.com/metarutaiga/xxYUV
//==============================================================================

template<int rgbWidth, bool interleaved, bool firstV>
void yuv2rgb(int width, int height, const void* y, const void* u, const void* v, int strideY, int strideU, int strideV, void* rgb, int strideRGB);

void yuv2rgb_yu12(int width, int height, const void* yuv, void* rgb, int rgbWidth = 3, int strideRGB = 0);
void yuv2rgb_yv12(int width, int height, const void* yuv, void* rgb, int rgbWidth = 3, int strideRGB = 0);
void yuv2rgb_nv12(int width, int height, const void* yuv, void* rgb, int rgbWidth = 3, int strideRGB = 0);
void yuv2rgb_nv21(int width, int height, const void* yuv, void* rgb, int rgbWidth = 3, int strideRGB = 0);
