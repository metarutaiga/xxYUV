//==============================================================================
// xxYUV : rgb2yuv Source
//
// Copyright (c) 2020 TAiGA
// https://github.com/metarutaiga/xxYUV
//==============================================================================
#include "rgb2yuv.h"

#define align(v, a) ((v) + ((a) - 1) & ~((a) - 1))

//------------------------------------------------------------------------------
template<int rgbWidth, bool interleaved, bool firstV>
void rgb2yuv(int width, int height, const void* rgb, int strideRGB, void* y, void* u, void* v, int strideY, int strideU, int strideV)
{
    int halfWidth = width >> 1;
    int halfHeight = height >> 1;

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
            int r00 = (rgbWidth >= 1) ? (*rgb0++) : 255;
            int g00 = (rgbWidth >= 2) ? (*rgb0++) : 255;
            int b00 = (rgbWidth >= 3) ? (*rgb0++) : 255;
            int a00 = (rgbWidth >= 4) ? (*rgb0++) : 255;
            int r01 = (rgbWidth >= 1) ? (*rgb0++) : 255;
            int g01 = (rgbWidth >= 2) ? (*rgb0++) : 255;
            int b01 = (rgbWidth >= 3) ? (*rgb0++) : 255;
            int a01 = (rgbWidth >= 4) ? (*rgb0++) : 255;
            int r10 = (rgbWidth >= 1) ? (*rgb1++) : 255;
            int g10 = (rgbWidth >= 2) ? (*rgb1++) : 255;
            int b10 = (rgbWidth >= 3) ? (*rgb1++) : 255;
            int a10 = (rgbWidth >= 4) ? (*rgb1++) : 255;
            int r11 = (rgbWidth >= 1) ? (*rgb1++) : 255;
            int g11 = (rgbWidth >= 2) ? (*rgb1++) : 255;
            int b11 = (rgbWidth >= 3) ? (*rgb1++) : 255;
            int a11 = (rgbWidth >= 4) ? (*rgb1++) : 255;

            int r000 = (r00 + r01 + r10 + r11) / 4;
            int g000 = (g00 + g01 + g10 + g11) / 4;
            int b000 = (b00 + b01 + b10 + b11) / 4;

            // BT.709
            //     R        G        B
            // Y =  0.21260  0.71520  0.07220
            // U = -0.09991 -0.33609  0.43600
            // V =  0.61500 -0.55861 -0.05639
            int y00 = r00 * (int)(0.21260 * 128) + g00 * (int)(0.71520 * 128) + b00 * (int)(0.07220 * 128);
            int y01 = r01 * (int)(0.21260 * 128) + g01 * (int)(0.71520 * 128) + b01 * (int)(0.07220 * 128);
            int y10 = r10 * (int)(0.21260 * 128) + g10 * (int)(0.71520 * 128) + b10 * (int)(0.07220 * 128);
            int y11 = r11 * (int)(0.21260 * 128) + g11 * (int)(0.71520 * 128) + b11 * (int)(0.07220 * 128);
            int u00 = r000 * (int)(-0.09991 * 128) + g000 * (int)(-0.33609 * 128) + b000 * (int)( 0.43600 * 128);
            int v00 = r000 * (int)( 0.61500 * 128) + g000 * (int)(-0.55861 * 128) + b000 * (int)(-0.05639 * 128);

            auto clamp = [](int value) -> unsigned char
            {
                return (unsigned char)(value < 255 ? value < 0 ? 0 : value : 255);
            };

            (*y0++) = clamp(y00 >> 7);
            (*y0++) = clamp(y01 >> 7);
            (*y1++) = clamp(y10 >> 7);
            (*y1++) = clamp(y11 >> 7);
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
void rgb2yuv_yu12(int width, int height, const void* rgb, void* yuv, int rgbWidth, int strideRGB, int alignWidth, int alignHeight)
{
    int sizeY = align(width, alignWidth) * align(height, alignHeight);
    int sizeUV = align(width / 2, alignWidth) * align(height / 2, alignHeight);

    if (strideRGB == 0)
        strideRGB = rgbWidth * width;

    if (rgbWidth == 3)
        rgb2yuv<3, false, false>(width, height, rgb, strideRGB, yuv, (char*)yuv + sizeY, (char*)yuv + sizeY + sizeUV, width, width / 2, width / 2);
    else if (rgbWidth == 4)
        rgb2yuv<4, false, false>(width, height, rgb, strideRGB, yuv, (char*)yuv + sizeY, (char*)yuv + sizeY + sizeUV, width, width / 2, width / 2);
}
//------------------------------------------------------------------------------
void rgb2yuv_yv12(int width, int height, const void* rgb, void* yuv, int rgbWidth, int strideRGB, int alignWidth, int alignHeight)
{
    int sizeY = align(width, alignWidth) * align(height, alignHeight);
    int sizeUV = align(width / 2, alignWidth) * align(height / 2, alignHeight);

    if (strideRGB == 0)
        strideRGB = rgbWidth * width;

    if (rgbWidth == 3)
        rgb2yuv<3, false, true>(width, height, rgb, strideRGB, yuv, (char*)yuv + sizeY + sizeUV, (char*)yuv + sizeY, width, width / 2, width / 2);
    else if (rgbWidth == 4)
        rgb2yuv<4, false, true>(width, height, rgb, strideRGB, yuv, (char*)yuv + sizeY + sizeUV, (char*)yuv + sizeY, width, width / 2, width / 2);
}
//------------------------------------------------------------------------------
void rgb2yuv_nv12(int width, int height, const void* rgb, void* yuv, int rgbWidth, int strideRGB, int alignWidth, int alignHeight)
{
    int sizeY = align(width, alignWidth) * align(height, alignHeight);

    if (strideRGB == 0)
        strideRGB = rgbWidth * width;

    if (rgbWidth == 3)
        rgb2yuv<3, true, false>(width, height, rgb, strideRGB, yuv, (char*)yuv + sizeY, (char*)yuv + sizeY + 1, width, width, width);
    else if (rgbWidth == 4)
        rgb2yuv<4, true, false>(width, height, rgb, strideRGB, yuv, (char*)yuv + sizeY, (char*)yuv + sizeY + 1, width, width, width);
}
//------------------------------------------------------------------------------
void rgb2yuv_nv21(int width, int height, const void* rgb, void* yuv, int rgbWidth, int strideRGB, int alignWidth, int alignHeight)
{
    int sizeY = align(width, alignWidth) * align(height, alignHeight);

    if (strideRGB == 0)
        strideRGB = rgbWidth * width;

    if (rgbWidth == 3)
        rgb2yuv<3, true, true>(width, height, rgb, strideRGB, yuv, (char*)yuv + sizeY + 1, (char*)yuv + sizeY, width, width, width);
    else if (rgbWidth == 4)
        rgb2yuv<4, true, true>(width, height, rgb, strideRGB, yuv, (char*)yuv + sizeY + 1, (char*)yuv + sizeY, width, width, width);
}
//------------------------------------------------------------------------------
