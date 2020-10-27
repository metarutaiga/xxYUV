//==============================================================================
// xxYUV : yuv Source
//
// Copyright (c) 2020 TAiGA
// https://github.com/metarutaiga/xxYUV
//==============================================================================
#include "yuv.h"

#if defined(__llvm__)
#   pragma clang diagnostic ignored "-Wunused-variable"
#endif

#define align(v, a) ((v) + ((a) - 1) & ~((a) - 1))

//------------------------------------------------------------------------------
template<bool interleaved, bool firstU, int iY, int iU, int iV, int iA>
void yuv(int width, int height, const void* y, const void* u, const void* v, int strideY, int strideU, int strideV, void* output, int strideOutput)
{
    if (strideOutput < 0)
    {
        output = (char*)output - (strideOutput * (height - 1));
    }

    int halfWidth = width >> 1;
    int halfHeight = height >> 1;

    for (int h = 0; h < halfHeight; ++h)
    {
        const unsigned char* y0 = (unsigned char*)y;
        const unsigned char* y1 = y0 + strideY;             y = y1 + strideY;
        const unsigned char* u0 = (unsigned char*)u;        u = u0 + strideU;
        const unsigned char* v0 = (unsigned char*)v;        v = v0 + strideV;
        unsigned char* output0 = (unsigned char*)output;
        unsigned char* output1 = output0 + strideOutput;    output = output1 + strideOutput;

        for (int w = 0; w < halfWidth; ++w)
        {
            auto y00 = (*y0++);
            auto y01 = (*y0++);
            auto y10 = (*y1++);
            auto y11 = (*y1++);

            auto u00 = (*u0++);
            auto v00 = (*v0++);
            if (interleaved)
            {
                u0++;
                v0++;
            }

            output0[iY] = y00;
            output0[iU] = u00;
            output0[iV] = v00;
            output0[iA] = 255;
            output0 += 4;

            output0[iY] = y01;
            output0[iU] = u00;
            output0[iV] = v00;
            output0[iA] = 255;
            output0 += 4;

            output1[iY] = y10;
            output1[iU] = u00;
            output1[iV] = v00;
            output1[iA] = 255;
            output1 += 4;

            output1[iY] = y11;
            output1[iU] = u00;
            output1[iV] = v00;
            output1[iA] = 255;
            output1 += 4;
        }
    }
}
//------------------------------------------------------------------------------
void yuv_yu12_to_vuya(int width, int height, const void* input, void* output, int strideOutput, int alignWidth, int alignHeight, int alignSize)
{
    int strideY = align(width, alignWidth);
    int strideU = align(width, alignWidth) / 2;
    int sizeY = align(strideY * align(height, alignHeight), alignSize);
    int sizeU = align(strideU * align(height, alignHeight) / 2, alignSize);

    if (strideOutput == 0)
        strideOutput = 4 * width;

    yuv<false, false, 2, 1, 0, 3>(width, height, input, (char*)input + sizeY, (char*)input + sizeY + sizeU, strideY, strideU, strideU, output, strideOutput);
}
//------------------------------------------------------------------------------
void yuv_yv12_to_vuya(int width, int height, const void* input, void* output, int strideOutput, int alignWidth, int alignHeight, int alignSize)
{
    int strideY = align(width, alignWidth);
    int strideU = align(width, alignWidth) / 2;
    int sizeY = align(strideY * align(height, alignHeight), alignSize);
    int sizeU = align(strideU * align(height, alignHeight) / 2, alignSize);

    if (strideOutput == 0)
        strideOutput = 4 * width;

    yuv<false, false, 2, 1, 0, 3>(width, height, input, (char*)input + sizeY + sizeU, (char*)input + sizeY, strideY, strideU, strideU, output, strideOutput);
}
//------------------------------------------------------------------------------
void yuv_nv12_to_vuya(int width, int height, const void* input, void* output, int strideOutput, int alignWidth, int alignHeight, int alignSize)
{
    int strideYUV = align(width, alignWidth);
    int sizeY = align(strideYUV * align(height, alignHeight), alignSize);
    int sizeUV = align(strideYUV * align(height, alignHeight) / 2, alignSize);

    if (strideOutput == 0)
        strideOutput = 4 * width;

    yuv<true, true, 2, 1, 0, 3>(width, height, input, (char*)input + sizeY, (char*)input + sizeY + 1, strideYUV, strideYUV, strideYUV, output, strideOutput);
}
//------------------------------------------------------------------------------
void yuv_nv21_to_vuya(int width, int height, const void* input, void* output, int strideOutput, int alignWidth, int alignHeight, int alignSize)
{
    int strideYUV = align(width, alignWidth);
    int sizeY = align(strideYUV * align(height, alignHeight), alignSize);
    int sizeUV = align(strideYUV * align(height, alignHeight) / 2, alignSize);

    if (strideOutput == 0)
        strideOutput = 4 * width;

    yuv<true, false, 2, 1, 0, 3>(width, height, input, (char*)input + sizeY + 1, (char*)input + sizeY, strideYUV, strideYUV, strideYUV, output, strideOutput);
}
//------------------------------------------------------------------------------
