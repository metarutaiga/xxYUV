//==============================================================================
// xxYUV : rgb2yuv Source
//
// Copyright (c) 2020 TAiGA
// https://github.com/metarutaiga/xxYUV
//==============================================================================
#if defined(__ARM_NEON__) || defined(__ARM_NEON) || defined(_M_ARM) || defined(_M_ARM64)
#   include <arm_neon.h>
#elif defined(_M_IX86) || defined(_M_AMD64) || defined(__i386__) || defined(__amd64__)
#   include <immintrin.h>
#   define _MM_DEINTERLACE4_EPI8(R0, R1, R2, R3) {  \
        __m128i T0, T1, T2, T3;                     \
        T0 = _mm_unpacklo_epi8(R0, R1);             \
        T1 = _mm_unpacklo_epi8(R2, R3);             \
        T2 = _mm_unpackhi_epi8(R0, R1);             \
        T3 = _mm_unpackhi_epi8(R2, R3);             \
        R0 = _mm_unpacklo_epi8(T0, T2);             \
        R1 = _mm_unpackhi_epi8(T0, T2);             \
        R2 = _mm_unpacklo_epi8(T1, T3);             \
        R3 = _mm_unpackhi_epi8(T1, T3);             \
        T0 = _mm_unpacklo_epi32(R0, R2);            \
        T1 = _mm_unpackhi_epi32(R0, R2);            \
        T2 = _mm_unpacklo_epi32(R1, R3);            \
        T3 = _mm_unpackhi_epi32(R1, R3);            \
        R0 = _mm_unpacklo_epi8(T0, T2);             \
        R1 = _mm_unpackhi_epi8(T0, T2);             \
        R2 = _mm_unpacklo_epi8(T1, T3);             \
        R3 = _mm_unpackhi_epi8(T1, T3);             \
    }
#endif
#include "rgb2yuv.h"

#if defined(__llvm__)
#   pragma clang diagnostic ignored "-Wunused-variable"
#endif

#define align(v, a) ((v) + ((a) - 1) & ~((a) - 1))

// BT.709 - Video Range
//      R        G        B
// Y =  0.18275  0.61477  0.06200
// U = -0.10072 -0.33882  0.43931
// V =  0.43867 -0.40048 -0.04038
//
// BT.709 - Full Range
//      R        G        B
// Y =  0.21260  0.71520  0.07220
// U = -0.11412 -0.38392  0.49804
// V =  0.49804 -0.45237 -0.04567
#define fRY  0.21260
#define fGY  0.71520
#define fBY  0.07220
#define fRU -0.11412
#define fGU -0.38392
#define fBU  0.49804
#define fRV  0.49804
#define fGV -0.45237
#define fBV -0.04567
#define vRY  0.18275
#define vGY  0.61477
#define vBY  0.06200
#define vRU -0.10072
#define vGU -0.33882
#define vBU  0.43931
#define vRV  0.43867
#define vGV -0.40048
#define vBV -0.04038

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

            uint8x8_t r00 = vget_low_u8(rgb00.val[iR]);
            uint8x8_t g00 = vget_low_u8(rgb00.val[iG]);
            uint8x8_t b00 = vget_low_u8(rgb00.val[iB]);
            uint8x8_t r01 = vget_high_u8(rgb00.val[iR]);
            uint8x8_t g01 = vget_high_u8(rgb00.val[iG]);
            uint8x8_t b01 = vget_high_u8(rgb00.val[iB]);
            uint8x8_t r10 = vget_low_u8(rgb10.val[iR]);
            uint8x8_t g10 = vget_low_u8(rgb10.val[iG]);
            uint8x8_t b10 = vget_low_u8(rgb10.val[iB]);
            uint8x8_t r11 = vget_high_u8(rgb10.val[iR]);
            uint8x8_t g11 = vget_high_u8(rgb10.val[iG]);
            uint8x8_t b11 = vget_high_u8(rgb10.val[iB]);

            uint16x8_t y00;
            uint16x8_t y01;
            uint16x8_t y10;
            uint16x8_t y11;
            if (fullRange == false)
            {
                y00 = vshrq_n_u16(vmlal_u8(vmlal_u8(vmlal_u8(vdupq_n_u16(16 << 8), r00, vdup_n_u8(RY)), g00, vdup_n_u8(GY)), b00, vdup_n_u8(BY)), 8);
                y01 = vshrq_n_u16(vmlal_u8(vmlal_u8(vmlal_u8(vdupq_n_u16(16 << 8), r01, vdup_n_u8(RY)), g01, vdup_n_u8(GY)), b01, vdup_n_u8(BY)), 8);
                y10 = vshrq_n_u16(vmlal_u8(vmlal_u8(vmlal_u8(vdupq_n_u16(16 << 8), r10, vdup_n_u8(RY)), g10, vdup_n_u8(GY)), b10, vdup_n_u8(BY)), 8);
                y11 = vshrq_n_u16(vmlal_u8(vmlal_u8(vmlal_u8(vdupq_n_u16(16 << 8), r11, vdup_n_u8(RY)), g11, vdup_n_u8(GY)), b11, vdup_n_u8(BY)), 8);
            }
            else
            {
                y00 = vshrq_n_u16(vmlal_u8(vmlal_u8(vmull_u8(r00, vdup_n_u8(RY)), g00, vdup_n_u8(GY)), b00, vdup_n_u8(BY)), 8);
                y01 = vshrq_n_u16(vmlal_u8(vmlal_u8(vmull_u8(r01, vdup_n_u8(RY)), g01, vdup_n_u8(GY)), b01, vdup_n_u8(BY)), 8);
                y10 = vshrq_n_u16(vmlal_u8(vmlal_u8(vmull_u8(r10, vdup_n_u8(RY)), g10, vdup_n_u8(GY)), b10, vdup_n_u8(BY)), 8);
                y11 = vshrq_n_u16(vmlal_u8(vmlal_u8(vmull_u8(r11, vdup_n_u8(RY)), g11, vdup_n_u8(GY)), b11, vdup_n_u8(BY)), 8);
            }
            uint8x16_t y000 = vcombine_u8(vqmovn_u16(y00), vqmovn_u16(y01));
            uint8x16_t y100 = vcombine_u8(vqmovn_u16(y10), vqmovn_u16(y11));

            int16x8_t r000 = vpadalq_u8(vpaddlq_u8(rgb00.val[iR]), rgb10.val[iR]);
            int16x8_t g000 = vpadalq_u8(vpaddlq_u8(rgb00.val[iG]), rgb10.val[iG]);
            int16x8_t b000 = vpadalq_u8(vpaddlq_u8(rgb00.val[iB]), rgb10.val[iB]);

            uint8x8_t u00 = vrshrn_n_s16(vmlaq_s16(vmlaq_s16(vmlaq_s16(vdupq_n_u16(128 << 8), r000, vdupq_n_s16(RU >> 2)), g000, vdupq_n_s16(GU >> 2)), b000, vdupq_n_s16(BU >> 2)), 8);
            uint8x8_t v00 = vrshrn_n_s16(vmlaq_s16(vmlaq_s16(vmlaq_s16(vdupq_n_u16(128 << 8), r000, vdupq_n_s16(RV >> 2)), g000, vdupq_n_s16(GV >> 2)), b000, vdupq_n_s16(BV >> 2)), 8);

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
                vst1_u8(u0, v00); u0 += 8;
                vst1_u8(v0, u00); v0 += 8;
            }
        }
        if (rgbWidth == 4)
            continue;
#elif defined(_M_IX86) || defined(_M_AMD64) || defined(__i386__) || defined(__amd64__)
        int halfWidth8 = (rgbWidth == 4) ? halfWidth / 8 : 0;
        for (int w = 0; w < halfWidth8; ++w)
        {
            __m128i rgb00[4] = { _mm_loadu_si128((__m128i*)rgb0), _mm_loadu_si128((__m128i*)rgb0 + 1), _mm_loadu_si128((__m128i*)rgb0 + 2), _mm_loadu_si128((__m128i*)rgb0 + 3) };  rgb0 += 16 * 4;
            __m128i rgb10[4] = { _mm_loadu_si128((__m128i*)rgb1), _mm_loadu_si128((__m128i*)rgb1 + 1), _mm_loadu_si128((__m128i*)rgb1 + 2), _mm_loadu_si128((__m128i*)rgb1 + 3) };  rgb1 += 16 * 4;
            _MM_DEINTERLACE4_EPI8(rgb00[0], rgb00[1], rgb00[2], rgb00[3]);
            _MM_DEINTERLACE4_EPI8(rgb10[0], rgb10[1], rgb10[2], rgb10[3]);

            __m128i r00 = _mm_unpacklo_epi8(rgb00[iR], __m128i());
            __m128i g00 = _mm_unpacklo_epi8(rgb00[iG], __m128i());
            __m128i b00 = _mm_unpacklo_epi8(rgb00[iB], __m128i());
            __m128i r01 = _mm_unpackhi_epi8(rgb00[iR], __m128i());
            __m128i g01 = _mm_unpackhi_epi8(rgb00[iG], __m128i());
            __m128i b01 = _mm_unpackhi_epi8(rgb00[iB], __m128i());
            __m128i r10 = _mm_unpacklo_epi8(rgb10[iR], __m128i());
            __m128i g10 = _mm_unpacklo_epi8(rgb10[iG], __m128i());
            __m128i b10 = _mm_unpacklo_epi8(rgb10[iB], __m128i());
            __m128i r11 = _mm_unpackhi_epi8(rgb10[iR], __m128i());
            __m128i g11 = _mm_unpackhi_epi8(rgb10[iG], __m128i());
            __m128i b11 = _mm_unpackhi_epi8(rgb10[iB], __m128i());

            __m128i y00 = _mm_add_epi16(_mm_add_epi16(_mm_mullo_epi16(r00, _mm_set1_epi16(RY)), _mm_mullo_epi16(g00, _mm_set1_epi16(GY))), _mm_mullo_epi16(b00, _mm_set1_epi16(BY)));
            __m128i y01 = _mm_add_epi16(_mm_add_epi16(_mm_mullo_epi16(r01, _mm_set1_epi16(RY)), _mm_mullo_epi16(g01, _mm_set1_epi16(GY))), _mm_mullo_epi16(b01, _mm_set1_epi16(BY)));
            __m128i y10 = _mm_add_epi16(_mm_add_epi16(_mm_mullo_epi16(r10, _mm_set1_epi16(RY)), _mm_mullo_epi16(g10, _mm_set1_epi16(GY))), _mm_mullo_epi16(b10, _mm_set1_epi16(BY)));
            __m128i y11 = _mm_add_epi16(_mm_add_epi16(_mm_mullo_epi16(r11, _mm_set1_epi16(RY)), _mm_mullo_epi16(g11, _mm_set1_epi16(GY))), _mm_mullo_epi16(b11, _mm_set1_epi16(BY)));
            y00 = _mm_srli_epi16(y00, 8);
            y01 = _mm_srli_epi16(y01, 8);
            y10 = _mm_srli_epi16(y10, 8);
            y11 = _mm_srli_epi16(y11, 8);
            if (fullRange == false)
            {
                y00 = _mm_add_epi16(y00, _mm_set1_epi16(16));
                y01 = _mm_add_epi16(y01, _mm_set1_epi16(16));
                y10 = _mm_add_epi16(y10, _mm_set1_epi16(16));
                y11 = _mm_add_epi16(y11, _mm_set1_epi16(16));
            }
            __m128i y000 = _mm_packus_epi16(y00, y01);
            __m128i y100 = _mm_packus_epi16(y10, y11);

            __m128i r000 = _mm_avg_epu8(rgb00[iR], rgb10[iR]);
            __m128i g000 = _mm_avg_epu8(rgb00[iG], rgb10[iG]);
            __m128i b000 = _mm_avg_epu8(rgb00[iB], rgb10[iB]);
            r000 = _mm_add_epi16(_mm_and_si128(r000, _mm_set1_epi16(0xFF)), _mm_srli_epi16(r000, 8));
            g000 = _mm_add_epi16(_mm_and_si128(g000, _mm_set1_epi16(0xFF)), _mm_srli_epi16(g000, 8));
            b000 = _mm_add_epi16(_mm_and_si128(b000, _mm_set1_epi16(0xFF)), _mm_srli_epi16(b000, 8));

            __m128i u00 = _mm_add_epi16(_mm_add_epi16(_mm_mullo_epi16(r000, _mm_set1_epi16(RU >> 1)), _mm_mullo_epi16(g000, _mm_set1_epi16(GU >> 1))), _mm_mullo_epi16(b000, _mm_set1_epi16(BU >> 1)));
            __m128i v00 = _mm_add_epi16(_mm_add_epi16(_mm_mullo_epi16(r000, _mm_set1_epi16(RV >> 1)), _mm_mullo_epi16(g000, _mm_set1_epi16(GV >> 1))), _mm_mullo_epi16(b000, _mm_set1_epi16(BV >> 1)));
            u00 = _mm_srai_epi16(u00, 8);
            v00 = _mm_srai_epi16(v00, 8);
            u00 = _mm_add_epi16(u00, _mm_set1_epi16(128));
            v00 = _mm_add_epi16(v00, _mm_set1_epi16(128));
            u00 = _mm_packus_epi16(u00, __m128());
            v00 = _mm_packus_epi16(v00, __m128());

            _mm_storeu_si128((__m128i*)y0, y000); y0 += 16;
            _mm_storeu_si128((__m128i*)y1, y100); y1 += 16;
            if (interleaved)
            {
                if (firstU)
                {
                    __m128i uv00 = _mm_unpacklo_epi8(v00, u00);
                    _mm_storeu_si128((__m128i*)u0, uv00); u0 += 16;
                }
                else
                {
                    __m128i uv00 = _mm_unpacklo_epi8(u00, v00);
                    _mm_storeu_si128((__m128i*)v0, uv00); v0 += 16;
                }
            }
            else
            {
                _mm_storeu_si64(u0, v00); u0 += 8;
                _mm_storeu_si64(v0, u00); v0 += 8;
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

            if (fullRange == false)
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
