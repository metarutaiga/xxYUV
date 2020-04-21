//==============================================================================
// xxYUV : rgb2yuv Source
//
// Copyright (c) 2020 TAiGA
// https://github.com/metarutaiga/xxYUV
//==============================================================================
#include "rgb2yuv.h"

#define VIDEO_RANGE 0
#define align(v, a) ((v) + ((a) - 1) & ~((a) - 1))

// BT.709 - Video Range
//      R        G        B
// Y =  0.21260  0.71520  0.07220
// U = -0.11412 -0.38392  0.49804
// V =  0.49804 -0.45237 -0.04567
//
// BT.709 - Full Range
//      R        G        B
// Y =  0.18275  0.61477  0.06200
// U = -0.10072 -0.33882  0.43931
// V =  0.43867 -0.40048 -0.04038
#if VIDEO_RANGE
#define RY  0.21260
#define GY  0.71520
#define BY  0.07220
#define RU -0.11412
#define GU -0.38392
#define BU  0.49804
#define RV  0.49804
#define GV -0.45237
#define BV -0.04567
#else
#define RY  0.18275
#define GY  0.61477
#define BY  0.06200
#define RU -0.10072
#define GU -0.33882
#define BU  0.43931
#define RV  0.43867
#define GV -0.40048
#define BV -0.04038
#endif

//------------------------------------------------------------------------------
template<int rgbWidth, bool rgbSwizzle, bool interleaved, bool firstV>
void rgb2yuv(int width, int height, const void* rgb, int strideRGB, void* y, void* u, void* v, int strideY, int strideU, int strideV)
{
    int halfWidth = width >> 1;
    int halfHeight = height >> 1;

    int iR = rgbSwizzle ? 2 : 0;
    int iG = 1;
    int iB = rgbSwizzle ? 0 : 2;
    int iA = 3;

    for (int h = 0; h < halfHeight; ++h)
    {
        const unsigned char* rgb0 = (unsigned char*)rgb;
        const unsigned char* rgb1 = rgb0 + strideRGB;       rgb = rgb1 + strideRGB;
        unsigned char* y0 = (unsigned char*)y;
        unsigned char* y1 = y0 + strideY;                   y = y1 + strideY;
        unsigned char* u0 = (unsigned char*)u;              u = u0 + strideU;
        unsigned char* v0 = (unsigned char*)v;              v = v0 + strideV;
        for (int w = 0; w < halfWidth; ++w)
        {
            int b00 = (rgbWidth >= 1) ? rgb0[iR] : 255;
            int g00 = (rgbWidth >= 2) ? rgb0[iG] : 255;
            int r00 = (rgbWidth >= 3) ? rgb0[iB] : 255;
            int a00 = (rgbWidth >= 4) ? rgb0[iA] : 255; rgb0 += rgbWidth;
            int b01 = (rgbWidth >= 1) ? rgb0[iR] : 255;
            int g01 = (rgbWidth >= 2) ? rgb0[iG] : 255;
            int r01 = (rgbWidth >= 3) ? rgb0[iB] : 255;
            int a01 = (rgbWidth >= 4) ? rgb0[iA] : 255; rgb0 += rgbWidth;
            int b10 = (rgbWidth >= 1) ? rgb1[iR] : 255;
            int g10 = (rgbWidth >= 2) ? rgb1[iG] : 255;
            int r10 = (rgbWidth >= 3) ? rgb1[iB] : 255;
            int a10 = (rgbWidth >= 4) ? rgb1[iA] : 255; rgb1 += rgbWidth;
            int b11 = (rgbWidth >= 1) ? rgb1[iR] : 255;
            int g11 = (rgbWidth >= 2) ? rgb1[iG] : 255;
            int r11 = (rgbWidth >= 3) ? rgb1[iB] : 255;
            int a11 = (rgbWidth >= 4) ? rgb1[iA] : 255; rgb1 += rgbWidth;

            int r000 = (r00 + r01 + r10 + r11) / 4;
            int g000 = (g00 + g01 + g10 + g11) / 4;
            int b000 = (b00 + b01 + b10 + b11) / 4;

            // BT.709
            //      R        G        B
            // Y =  0.18275  0.61477  0.06200
            // U = -0.10072 -0.33882  0.43931
            // V =  0.43867 -0.40048 -0.04038
            int y00 = r00 * (int)(0.18275 * 128) + g00 * (int)(0.61477 * 128) + b00 * (int)(0.06200 * 128);
            int y01 = r01 * (int)(0.18275 * 128) + g01 * (int)(0.61477 * 128) + b01 * (int)(0.06200 * 128);
            int y10 = r10 * (int)(0.18275 * 128) + g10 * (int)(0.61477 * 128) + b10 * (int)(0.06200 * 128);
            int y11 = r11 * (int)(0.18275 * 128) + g11 * (int)(0.61477 * 128) + b11 * (int)(0.06200 * 128);
            int u00 = r000 * (int)(-0.10072 * 128) + g000 * (int)(-0.33882 * 128) + b000 * (int)( 0.43931 * 128);
            int v00 = r000 * (int)( 0.43867 * 128) + g000 * (int)(-0.40048 * 128) + b000 * (int)(-0.04038 * 128);

            auto clamp = [](int value) -> unsigned char
            {
                return (unsigned char)(value < 255 ? value < 0 ? 0 : value : 255);
            };

#if VIDEO_RANGE
            (*y0++) = clamp(y00 >> 7);
            (*y0++) = clamp(y01 >> 7);
            (*y1++) = clamp(y10 >> 7);
            (*y1++) = clamp(y11 >> 7);
#else
            (*y0++) = clamp((y00 >> 7) + 16);
            (*y0++) = clamp((y01 >> 7) + 16);
            (*y1++) = clamp((y10 >> 7) + 16);
            (*y1++) = clamp((y11 >> 7) + 16);
#endif
            (*u0++) = clamp((u00 >> 7) + 128);
            (*v0++) = clamp((v00 >> 7) + 128);
            if (interleaved)
            {
                u0++;
                v0++;
            }
        }
    }
}
//------------------------------------------------------------------------------
void rgb2yuv_yu12(int width, int height, const void* rgb, void* yuv, int rgbWidth, bool rgbSwizzle, int strideRGB, int alignWidth, int alignHeight)
{
    int sizeY = align(width, alignWidth) * align(height, alignHeight);
    int sizeUV = align(width / 2, alignWidth) * align(height / 2, alignHeight);

    if (strideRGB == 0)
        strideRGB = rgbWidth * width;

    if (rgbSwizzle)
    {
        if (rgbWidth == 3)
            rgb2yuv<3, true, false, false>(width, height, rgb, strideRGB, yuv, (char*)yuv + sizeY, (char*)yuv + sizeY + sizeUV, width, width / 2, width / 2);
        else if (rgbWidth == 4)
            rgb2yuv<4, true, false, false>(width, height, rgb, strideRGB, yuv, (char*)yuv + sizeY, (char*)yuv + sizeY + sizeUV, width, width / 2, width / 2);
    }
    else
    {
        if (rgbWidth == 3)
            rgb2yuv<3, false, false, false>(width, height, rgb, strideRGB, yuv, (char*)yuv + sizeY, (char*)yuv + sizeY + sizeUV, width, width / 2, width / 2);
        else if (rgbWidth == 4)
            rgb2yuv<4, false, false, false>(width, height, rgb, strideRGB, yuv, (char*)yuv + sizeY, (char*)yuv + sizeY + sizeUV, width, width / 2, width / 2);
    }
}
//------------------------------------------------------------------------------
void rgb2yuv_yv12(int width, int height, const void* rgb, void* yuv, int rgbWidth, bool rgbSwizzle, int strideRGB, int alignWidth, int alignHeight)
{
    int sizeY = align(width, alignWidth) * align(height, alignHeight);
    int sizeUV = align(width / 2, alignWidth) * align(height / 2, alignHeight);

    if (strideRGB == 0)
        strideRGB = rgbWidth * width;

    if (rgbSwizzle)
    {
        if (rgbWidth == 3)
            rgb2yuv<3, true, false, true>(width, height, rgb, strideRGB, yuv, (char*)yuv + sizeY + sizeUV, (char*)yuv + sizeY, width, width / 2, width / 2);
        else if (rgbWidth == 4)
            rgb2yuv<4, true, false, true>(width, height, rgb, strideRGB, yuv, (char*)yuv + sizeY + sizeUV, (char*)yuv + sizeY, width, width / 2, width / 2);
    }
    else
    {
        if (rgbWidth == 3)
            rgb2yuv<3, false, false, true>(width, height, rgb, strideRGB, yuv, (char*)yuv + sizeY + sizeUV, (char*)yuv + sizeY, width, width / 2, width / 2);
        else if (rgbWidth == 4)
            rgb2yuv<4, false, false, true>(width, height, rgb, strideRGB, yuv, (char*)yuv + sizeY + sizeUV, (char*)yuv + sizeY, width, width / 2, width / 2);
    }
}
//------------------------------------------------------------------------------
void rgb2yuv_nv12(int width, int height, const void* rgb, void* yuv, int rgbWidth, bool rgbSwizzle, int strideRGB, int alignWidth, int alignHeight)
{
    int sizeY = align(width, alignWidth) * align(height, alignHeight);

    if (strideRGB == 0)
        strideRGB = rgbWidth * width;

    if (rgbSwizzle)
    {
        if (rgbWidth == 3)
            rgb2yuv<3, true, true, false>(width, height, rgb, strideRGB, yuv, (char*)yuv + sizeY, (char*)yuv + sizeY + 1, width, width, width);
        else if (rgbWidth == 4)
            rgb2yuv<4, true, true, false>(width, height, rgb, strideRGB, yuv, (char*)yuv + sizeY, (char*)yuv + sizeY + 1, width, width, width);
    }
    else
    {
        if (rgbWidth == 3)
            rgb2yuv<3, false, true, false>(width, height, rgb, strideRGB, yuv, (char*)yuv + sizeY, (char*)yuv + sizeY + 1, width, width, width);
        else if (rgbWidth == 4)
            rgb2yuv<4, false, true, false>(width, height, rgb, strideRGB, yuv, (char*)yuv + sizeY, (char*)yuv + sizeY + 1, width, width, width);
    }
}
//------------------------------------------------------------------------------
void rgb2yuv_nv21(int width, int height, const void* rgb, void* yuv, int rgbWidth, bool rgbSwizzle, int strideRGB, int alignWidth, int alignHeight)
{
    int sizeY = align(width, alignWidth) * align(height, alignHeight);

    if (strideRGB == 0)
        strideRGB = rgbWidth * width;

    if (rgbSwizzle)
    {
        if (rgbWidth == 3)
            rgb2yuv<3, true, true, true>(width, height, rgb, strideRGB, yuv, (char*)yuv + sizeY + 1, (char*)yuv + sizeY, width, width, width);
        else if (rgbWidth == 4)
            rgb2yuv<4, true, true, true>(width, height, rgb, strideRGB, yuv, (char*)yuv + sizeY + 1, (char*)yuv + sizeY, width, width, width);
    }
    else
    {
        if (rgbWidth == 3)
            rgb2yuv<3, false, true, true>(width, height, rgb, strideRGB, yuv, (char*)yuv + sizeY + 1, (char*)yuv + sizeY, width, width, width);
        else if (rgbWidth == 4)
            rgb2yuv<4, false, true, true>(width, height, rgb, strideRGB, yuv, (char*)yuv + sizeY + 1, (char*)yuv + sizeY, width, width, width);
    }
}
//------------------------------------------------------------------------------
