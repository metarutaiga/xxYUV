//==============================================================================
// xxYUV : rgb2yuv Source
//
// Copyright (c) 2020 TAiGA
// https://github.com/metarutaiga/xxYUV
//==============================================================================
#if defined(__ARM_NEON__) || defined(__ARM_NEON) || defined(_M_ARM) || defined(_M_ARM64)
#   include <arm_neon.h>
#endif
#include "rgb2yuv.h"

#if defined(__llvm__)
#   pragma clang diagnostic ignored "-Wunused-variable"
#endif

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
#define vRY  0.21260
#define vGY  0.71520
#define vBY  0.07220
#define vRU -0.11412
#define vGU -0.38392
#define vBU  0.49804
#define vRV  0.49804
#define vGV -0.45237
#define vBV -0.04567
#define fRY  0.18275
#define fGY  0.61477
#define fBY  0.06200
#define fRU -0.10072
#define fGU -0.33882
#define fBU  0.43931
#define fRV  0.43867
#define fGV -0.40048
#define fBV -0.04038

//------------------------------------------------------------------------------
template<int rgbWidth, bool rgbSwizzle, bool interleaved, bool firstU, bool fullRange>
void rgb2yuv(int width, int height, const void* rgb, int strideRGB, void* y, void* u, void* v, int strideY, int strideU, int strideV)
{
    if (strideRGB < 0)
    {
        rgb = (char*)rgb - (strideRGB * (height - 1));
    }

    int halfWidth = width >> 1;
    int halfHeight = height >> 1;

    int iR = rgbSwizzle ? 2 : 0;
    int iG = 1;
    int iB = rgbSwizzle ? 0 : 2;
    int iA = 3;

    int RY, RU, RV, GY, GU, GV, BY, BU, BV;
    if (fullRange)
    {
        RY = (int)(fRY * 256); RU = (int)(fRU * 256); RV = (int)(fRV * 256);
        GY = (int)(fGY * 256); GU = (int)(fGU * 256); GV = (int)(fGV * 256);
        BY = (int)(fBY * 256); BU = (int)(fBU * 256); BV = (int)(fBV * 256);
    }
    else
    {
        RY = (int)(vRY * 256); RU = (int)(vRU * 256); RV = (int)(vRV * 256);
        GY = (int)(vGY * 256); GU = (int)(vGU * 256); GV = (int)(vGV * 256);
        BY = (int)(vBY * 256); BU = (int)(vBU * 256); BV = (int)(vBV * 256);
    }

    for (int h = 0; h < halfHeight; ++h)
    {
        const unsigned char* rgb0 = (unsigned char*)rgb;
        const unsigned char* rgb1 = rgb0 + strideRGB;       rgb = rgb1 + strideRGB;
        unsigned char* y0 = (unsigned char*)y;
        unsigned char* y1 = y0 + strideY;                   y = y1 + strideY;
        unsigned char* u0 = (unsigned char*)u;              u = u0 + strideU;
        unsigned char* v0 = (unsigned char*)v;              v = v0 + strideV;
#if defined(__ARM_NEON__) || defined(__ARM_NEON) || defined(_M_ARM) || defined(_M_ARM64)
        int halfWidth8 = (rgbWidth == 4) ? halfWidth / 8 : 0;
        for (int w = 0; w < halfWidth8; ++w)
        {
            uint8x16x4_t rgb00 = vld4q_u8(rgb0);  rgb0 += 16 * 4;
            uint8x16x4_t rgb10 = vld4q_u8(rgb1);  rgb1 += 16 * 4;

            int16x8_t r00 = vmovl_u8(vget_low_u8(rgb00.val[iR]));
            int16x8_t g00 = vmovl_u8(vget_low_u8(rgb00.val[iG]));
            int16x8_t b00 = vmovl_u8(vget_low_u8(rgb00.val[iB]));
            int16x8_t r01 = vmovl_u8(vget_high_u8(rgb00.val[iR]));
            int16x8_t g01 = vmovl_u8(vget_high_u8(rgb00.val[iG]));
            int16x8_t b01 = vmovl_u8(vget_high_u8(rgb00.val[iB]));
            int16x8_t r10 = vmovl_u8(vget_low_u8(rgb10.val[iR]));
            int16x8_t g10 = vmovl_u8(vget_low_u8(rgb10.val[iG]));
            int16x8_t b10 = vmovl_u8(vget_low_u8(rgb10.val[iB]));
            int16x8_t r11 = vmovl_u8(vget_high_u8(rgb10.val[iR]));
            int16x8_t g11 = vmovl_u8(vget_high_u8(rgb10.val[iG]));
            int16x8_t b11 = vmovl_u8(vget_high_u8(rgb10.val[iB]));

            int16x8_t y00 = vshrq_n_s16(vmlaq_s16(vmlaq_s16(vmulq_s16(r00, vdupq_n_s16(RY >> 1)), g00, vdupq_n_s16(GY >> 1)), b00, vdupq_n_s16(BY >> 1)), 7);
            int16x8_t y01 = vshrq_n_s16(vmlaq_s16(vmlaq_s16(vmulq_s16(r01, vdupq_n_s16(RY >> 1)), g01, vdupq_n_s16(GY >> 1)), b01, vdupq_n_s16(BY >> 1)), 7);
            int16x8_t y10 = vshrq_n_s16(vmlaq_s16(vmlaq_s16(vmulq_s16(r10, vdupq_n_s16(RY >> 1)), g10, vdupq_n_s16(GY >> 1)), b10, vdupq_n_s16(BY >> 1)), 7);
            int16x8_t y11 = vshrq_n_s16(vmlaq_s16(vmlaq_s16(vmulq_s16(r11, vdupq_n_s16(RY >> 1)), g11, vdupq_n_s16(GY >> 1)), b11, vdupq_n_s16(BY >> 1)), 7);
            uint8x16_t y000 = vcombine_u8(vqmovun_s16(y00), vqmovun_s16(y01));
            uint8x16_t y100 = vcombine_u8(vqmovun_s16(y10), vqmovun_s16(y11));

            int16x8_t r000 = vshrq_n_u16(vpadalq_u8(vpaddlq_u8(rgb00.val[iR]), rgb10.val[iR]), 2);
            int16x8_t g000 = vshrq_n_u16(vpadalq_u8(vpaddlq_u8(rgb00.val[iG]), rgb10.val[iG]), 2);
            int16x8_t b000 = vshrq_n_u16(vpadalq_u8(vpaddlq_u8(rgb00.val[iB]), rgb10.val[iB]), 2);

            uint8x8_t u00 = vreinterpret_u8_s8(vsub_s8(vrshrn_n_s16(vmlaq_s16(vmlaq_s16(vmulq_s16(r000, vdupq_n_s16(RU >> 1)), g000, vdupq_n_s16(GU >> 1)), b000, vdupq_n_s16(BU >> 1)), 7), vdup_n_s8(-128)));
            uint8x8_t v00 = vreinterpret_u8_s8(vsub_s8(vrshrn_n_s16(vmlaq_s16(vmlaq_s16(vmulq_s16(r000, vdupq_n_s16(RV >> 1)), g000, vdupq_n_s16(GV >> 1)), b000, vdupq_n_s16(BV >> 1)), 7), vdup_n_s8(-128)));

            vst1q_u8(y0, y000); y0 += 16;
            vst1q_u8(y1, y100); y1 += 16;
            if (interleaved)
            {
                if (firstU)
                {
                    uint8x8x2_t uv00 = vzip_u8(v00, u00);
                    vst1q_u8(u0, vcombine_u8(uv00.val[0], uv00.val[1])); u0 += 16;
                }
                else
                {
                    uint8x8x2_t uv00 = vzip_u8(u00, v00);
                    vst1q_u8(v0, vcombine_u8(uv00.val[0], uv00.val[1])); v0 += 16;
                }
            }
            else
            {
                vst1_u8(u0, u00); u0 += 8;
                vst1_u8(v0, v00); v0 += 8;
            }
        }
        if (rgbWidth == 4)
            continue;
#endif
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

            int y00 = r00  * RY + g00  * GY + b00  * BY;
            int y01 = r01  * RY + g01  * GY + b01  * BY;
            int y10 = r10  * RY + g10  * GY + b10  * BY;
            int y11 = r11  * RY + g11  * GY + b11  * BY;
            int u00 = r000 * RU + g000 * GU + b000 * BU;
            int v00 = r000 * RV + g000 * GV + b000 * BV;

            auto clamp = [](int value) -> unsigned char
            {
                return (unsigned char)(value < 255 ? value < 0 ? 0 : value : 255);
            };

            if (fullRange)
            {
                (*y0++) = clamp((y00 >> 8) + 16);
                (*y0++) = clamp((y01 >> 8) + 16);
                (*y1++) = clamp((y10 >> 8) + 16);
                (*y1++) = clamp((y11 >> 8) + 16);
            }
            else
            {
                (*y0++) = clamp(y00 >> 8);
                (*y0++) = clamp(y01 >> 8);
                (*y1++) = clamp(y10 >> 8);
                (*y1++) = clamp(y11 >> 8);
            }
            (*u0++) = clamp((u00 >> 8) + 128);
            (*v0++) = clamp((v00 >> 8) + 128);
            if (interleaved)
            {
                u0++;
                v0++;
            }
        }
    }
}
//------------------------------------------------------------------------------
void rgb2yuv_yu12(int width, int height, const void* rgb, void* yuv, int rgbWidth, bool rgbSwizzle, bool fullRange, int strideRGB, int alignWidth, int alignHeight, int alignSize)
{
    int strideY = align(width, alignWidth);
    int strideU = align(width, alignWidth) / 2;
    int sizeY = align(strideY * align(height, alignHeight), alignSize);
    int sizeU = align(strideU * align(height, alignHeight) / 2, alignSize);

    if (strideRGB == 0)
        strideRGB = rgbWidth * width;

    auto converter = rgb2yuv<3, false, false, false, false>;

    if (rgbWidth == 3)
    {
        if (rgbSwizzle)
        {
            if (fullRange)
                converter = rgb2yuv<3, true, false, false, true>;
            else
                converter = rgb2yuv<3, true, false, false, false>;
        }
        else
        {
            if (fullRange)
                converter = rgb2yuv<3, false, false, false, true>;
            else
                converter = rgb2yuv<3, false, false, false, false>;
        }
    }
    else if (rgbWidth == 4)
    {
        if (rgbSwizzle)
        {
            if (fullRange)
                converter = rgb2yuv<4, true, false, false, true>;
            else
                converter = rgb2yuv<4, true, false, false, false>;
        }
        else
        {
            if (fullRange)
                converter = rgb2yuv<4, false, false, false, true>;
            else
                converter = rgb2yuv<4, false, false, false, false>;
        }
    }

    converter(width, height, rgb, strideRGB, yuv, (char*)yuv + sizeY, (char*)yuv + sizeY + sizeU, strideY, strideU, strideU);
}
//------------------------------------------------------------------------------
void rgb2yuv_yv12(int width, int height, const void* rgb, void* yuv, int rgbWidth, bool rgbSwizzle, bool fullRange, int strideRGB, int alignWidth, int alignHeight, int alignSize)
{
    int strideY = align(width, alignWidth);
    int strideU = align(width, alignWidth) / 2;
    int sizeY = align(strideY * align(height, alignHeight), alignSize);
    int sizeU = align(strideU * align(height, alignHeight) / 2, alignSize);

    if (strideRGB == 0)
        strideRGB = rgbWidth * width;

    auto converter = rgb2yuv<3, false, false, false, false>;

    if (rgbWidth == 3)
    {
        if (rgbSwizzle)
        {
            if (fullRange)
                converter = rgb2yuv<3, true, false, false, true>;
            else
                converter = rgb2yuv<3, true, false, false, false>;
        }
        else
        {
            if (fullRange)
                converter = rgb2yuv<3, false, false, false, true>;
            else
                converter = rgb2yuv<3, false, false, false, false>;
        }
    }
    else if (rgbWidth == 4)
    {
        if (rgbSwizzle)
        {
            if (fullRange)
                converter = rgb2yuv<4, true, false, false, true>;
            else
                converter = rgb2yuv<4, true, false, false, false>;
        }
        else
        {
            if (fullRange)
                converter = rgb2yuv<4, false, false, false, true>;
            else
                converter = rgb2yuv<4, false, false, false, false>;
        }
    }

    converter(width, height, rgb, strideRGB, yuv, (char*)yuv + sizeY + sizeU, (char*)yuv + sizeY, strideY, strideU, strideU);
}
//------------------------------------------------------------------------------
void rgb2yuv_nv12(int width, int height, const void* rgb, void* yuv, int rgbWidth, bool rgbSwizzle, bool fullRange, int strideRGB, int alignWidth, int alignHeight, int alignSize)
{
    int strideYUV = align(width, alignWidth);
    int sizeY = align(strideYUV * align(height, alignHeight), alignSize);
    int sizeUV = align(strideYUV * align(height, alignHeight) / 2, alignSize);

    if (strideRGB == 0)
        strideRGB = rgbWidth * width;

    auto converter = rgb2yuv<3, false, false, false, false>;

    if (rgbWidth == 3)
    {
        if (rgbSwizzle)
        {
            if (fullRange)
                converter = rgb2yuv<3, true, true, true, true>;
            else
                converter = rgb2yuv<3, true, true, true, false>;
        }
        else
        {
            if (fullRange)
                converter = rgb2yuv<3, false, true, true, true>;
            else
                converter = rgb2yuv<3, false, true, true, false>;
        }
    }
    else if (rgbWidth == 4)
    {
        if (rgbSwizzle)
        {
            if (fullRange)
                converter = rgb2yuv<4, true, true, true, true>;
            else
                converter = rgb2yuv<4, true, true, true, false>;
        }
        else
        {
            if (fullRange)
                converter = rgb2yuv<4, false, true, true, true>;
            else
                converter = rgb2yuv<4, false, true, true, false>;
        }
    }

    converter(width, height, rgb, strideRGB, yuv, (char*)yuv + sizeY, (char*)yuv + sizeY + 1, strideYUV, strideYUV, strideYUV);
}
//------------------------------------------------------------------------------
void rgb2yuv_nv21(int width, int height, const void* rgb, void* yuv, int rgbWidth, bool rgbSwizzle, bool fullRange, int strideRGB, int alignWidth, int alignHeight, int alignSize)
{
    int strideYUV = align(width, alignWidth);
    int sizeY = align(strideYUV * align(height, alignHeight), alignSize);
    int sizeUV = align(strideYUV * align(height, alignHeight) / 2, alignSize);

    if (strideRGB == 0)
        strideRGB = rgbWidth * width;

    auto converter = rgb2yuv<3, false, false, false, false>;

    if (rgbWidth == 3)
    {
        if (rgbSwizzle)
        {
            if (fullRange)
                converter = rgb2yuv<3, true, true, false, true>;
            else
                converter = rgb2yuv<3, true, true, false, false>;
        }
        else
        {
            if (fullRange)
                converter = rgb2yuv<3, false, true, false, true>;
            else
                converter = rgb2yuv<3, false, true, false, false>;
        }
    }
    else if (rgbWidth == 4)
    {
        if (rgbSwizzle)
        {
            if (fullRange)
                converter = rgb2yuv<4, true, true, false, true>;
            else
                converter = rgb2yuv<4, true, true, false, false>;
        }
        else
        {
            if (fullRange)
                converter = rgb2yuv<4, false, true, false, true>;
            else
                converter = rgb2yuv<4, false, true, false, false>;
        }
    }

    converter(width, height, rgb, strideRGB, yuv, (char*)yuv + sizeY + 1, (char*)yuv + sizeY, strideYUV, strideYUV, strideYUV);
}
//------------------------------------------------------------------------------
