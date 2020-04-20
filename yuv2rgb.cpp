//==============================================================================
// xxYUV : yuv2rgb Source
//
// Copyright (c) 2020 TAiGA
// https://github.com/metarutaiga/xxYUV
//==============================================================================
#if defined(__ARM_NEON__) || defined(__ARM_NEON) || defined(_M_ARM) || defined(_M_ARM64)
#   include <arm_neon.h>
#elif defined(_M_IX86) || defined(_M_AMD64) || defined(__i386__) || defined(__amd64__)
#   include <emmintrin.h>
#   define _MM_TRANSPOSE4_EPI8(R0, R1, R2, R3) {    \
        __m128i T0, T1, T2, T3;                     \
        T0 = _mm_unpacklo_epi8(R0, R1);             \
        T1 = _mm_unpacklo_epi8(R2, R3);             \
        T2 = _mm_unpackhi_epi8(R0, R1);             \
        T3 = _mm_unpackhi_epi8(R2, R3);             \
        R0 = _mm_unpacklo_epi16(T0, T1);            \
        R1 = _mm_unpackhi_epi16(T0, T1);            \
        R2 = _mm_unpacklo_epi16(T2, T3);            \
        R3 = _mm_unpackhi_epi16(T2, T3);            \
    }
#   if defined(__AVX2__) && 0
#       include <immintrin.h>
#       define _MM256_TRANSPOSE4_EPI8(R0, R1, R2, R3) { \
            __m256i T0, T1, T2, T3;                     \
            T0 = _mm256_unpacklo_epi8(R0, R1);          \
            T1 = _mm256_unpacklo_epi8(R2, R3);          \
            T2 = _mm256_unpackhi_epi8(R0, R1);          \
            T3 = _mm256_unpackhi_epi8(R2, R3);          \
            R0 = _mm256_unpacklo_epi16(T0, T1);         \
            R1 = _mm256_unpackhi_epi16(T0, T1);         \
            R2 = _mm256_unpacklo_epi16(T2, T3);         \
            R3 = _mm256_unpackhi_epi16(T2, T3);         \
        }
#   endif
#endif
#include "yuv2rgb.h"

#define align(v, a) ((v) + ((a) - 1) & ~((a) - 1))

//------------------------------------------------------------------------------
template<int rgbWidth, bool rgbSwizzle, bool interleaved, bool firstV>
void yuv2rgb(int width, int height, const void* y, const void* u, const void* v, int strideY, int strideU, int strideV, void* rgb, int strideRGB)
{
    int halfWidth = width >> 1;
    int halfHeight = height >> 1;

    int iR = rgbSwizzle ? 2 : 0;
    int iG = 1;
    int iB = rgbSwizzle ? 0 : 2;
    int iA = 3;

    for (int h = 0; h < halfHeight; ++h)
    {
        const unsigned char* y0 = (unsigned char*)y;
        const unsigned char* y1 = y0 + strideY;         y = y1 + strideY;
        const unsigned char* u0 = (unsigned char*)u;    u = u0 + strideU;
        const unsigned char* v0 = (unsigned char*)v;    v = v0 + strideV;
        unsigned char* rgb0 = (unsigned char*)rgb;
        unsigned char* rgb1 = rgb0 + strideRGB;         rgb = rgb1 + strideRGB;
#if defined(__ARM_NEON__) || defined(__ARM_NEON) || defined(_M_ARM) || defined(_M_ARM64)
        int halfWidth8 = (rgbWidth == 4) ? halfWidth / 8 : 0;
        for (int w = 0; w < halfWidth8; ++w)
        {
            uint8x16_t y00lh = vld1q_u8(y0); y0 += 16;
            uint8x16_t y10lh = vld1q_u8(y1); y1 += 16;
            uint8x8_t y00 = vget_low_u8(y00lh);
            uint8x8_t y01 = vget_high_u8(y00lh);
            uint8x8_t y10 = vget_low_u8(y10lh);
            uint8x8_t y11 = vget_high_u8(y10lh);

            int16x8_t u00;
            int16x8_t v00;
            if (interleaved)
            {
                if (firstV)
                {
                    uint8x16_t uv00 = vld1q_u8(v0); v0 += 16;
                    uint8x8x2_t uv00lh = vuzp_u8(vget_low_u8(uv00), vget_high_u8(uv00));
                    u00 = vreinterpretq_s16_u16(vsubl_u8(uv00lh.val[1], vdup_n_u8(128)));
                    v00 = vreinterpretq_s16_u16(vsubl_u8(uv00lh.val[0], vdup_n_u8(128)));
                }
                else
                {
                    uint8x16_t uv00 = vld1q_u8(u0); u0 += 16;
                    uint8x8x2_t uv00lh = vuzp_u8(vget_low_u8(uv00), vget_high_u8(uv00));
                    u00 = vreinterpretq_s16_u16(vsubl_u8(uv00lh.val[0], vdup_n_u8(128)));
                    v00 = vreinterpretq_s16_u16(vsubl_u8(uv00lh.val[1], vdup_n_u8(128)));
                }
            }
            else
            {
                u00 = vreinterpretq_s16_u16(vsubl_u8(vld1_u8(u0), vdup_n_u8(128))); u0 += 8;
                v00 = vreinterpretq_s16_u16(vsubl_u8(vld1_u8(v0), vdup_n_u8(128))); v0 += 8;
            }

            int16x8_t dR = vshrq_n_s16(                                           vmulq_n_s16(v00, (short)( 1.28033 * 128)), 7);
            int16x8_t dG = vshrq_n_s16(vmlaq_n_s16(vmulq_n_s16(u00, (short)(-0.21482 * 256)), v00, (short)(-0.38059 * 256)), 8);
            int16x8_t dB = vshrq_n_s16(            vmulq_n_s16(u00, (short)( 2.12798 *  64)),                                6);

            int16x8x2_t xR = vzipq_s16(dR, dR);
            int16x8x2_t xG = vzipq_s16(dG, dG);
            int16x8x2_t xB = vzipq_s16(dB, dB);

            uint8x16x4_t t;
            uint8x16x4_t b;

            t.val[iR] = vcombine_u8(vqmovun_s16(vaddw_u8(vreinterpretq_u16_s16(xR.val[0]), y00)), vqmovun_s16(vaddw_u8(vreinterpretq_u16_s16(xR.val[1]), y01)));
            t.val[iG] = vcombine_u8(vqmovun_s16(vaddw_u8(vreinterpretq_u16_s16(xG.val[0]), y00)), vqmovun_s16(vaddw_u8(vreinterpretq_u16_s16(xG.val[1]), y01)));
            t.val[iB] = vcombine_u8(vqmovun_s16(vaddw_u8(vreinterpretq_u16_s16(xB.val[0]), y00)), vqmovun_s16(vaddw_u8(vreinterpretq_u16_s16(xB.val[1]), y01)));
            t.val[iA] = vdupq_n_u8(255);
            b.val[iR] = vcombine_u8(vqmovun_s16(vaddw_u8(vreinterpretq_u16_s16(xR.val[0]), y10)), vqmovun_s16(vaddw_u8(vreinterpretq_u16_s16(xR.val[1]), y11)));
            b.val[iG] = vcombine_u8(vqmovun_s16(vaddw_u8(vreinterpretq_u16_s16(xG.val[0]), y10)), vqmovun_s16(vaddw_u8(vreinterpretq_u16_s16(xG.val[1]), y11)));
            b.val[iB] = vcombine_u8(vqmovun_s16(vaddw_u8(vreinterpretq_u16_s16(xB.val[0]), y10)), vqmovun_s16(vaddw_u8(vreinterpretq_u16_s16(xB.val[1]), y11)));
            b.val[iA] = vdupq_n_u8(255);

            vst4q_u8(rgb0, t);  rgb0 += 16 * 4;
            vst4q_u8(rgb1, b);  rgb1 += 16 * 4;
        }
        if (rgbWidth == 4)
            continue;
#elif defined(_M_IX86) || defined(_M_AMD64) || defined(__i386__) || defined(__amd64__)
#if defined(__AVX2__) && 0
        int halfWidth16 = (rgbWidth == 4) ? halfWidth / 16 : 0;
        for (int w = 0; w < halfWidth16; ++w)
        {
            __m256i y00lh = _mm256_load_si256((__m256i*)y0);   y0 += 32;
            __m256i y10lh = _mm256_load_si256((__m256i*)y1);   y1 += 32;
            __m256i y00 = _mm256_unpacklo_epi8(y00lh, __m256i());
            __m256i y01 = _mm256_unpacklo_epi8(y00lh, __m256i());
            __m256i y10 = _mm256_unpacklo_epi8(y10lh, __m256i());
            __m256i y11 = _mm256_unpacklo_epi8(y10lh, __m256i());

            __m256i u00;
            __m256i v00;
            if (interleaved)
            {
                if (firstV)
                {
                    __m256i uv00 = _mm256_load_si256((__m256i*)v0); v0 += 32;
                    u00 = _mm256_sub_epi16(_mm256_srli_epi16(uv00, 8), _mm256_set1_epi16(128));
                    v00 = _mm256_sub_epi16(_mm256_and_si256(uv00, _mm256_set1_epi16(0xFF)), _mm256_set1_epi16(128));
                }
                else
                {
                    __m256i uv00 = _mm256_load_si256((__m256i*)u0); u0 += 32;
                    u00 = _mm256_sub_epi16(_mm256_and_si256(uv00, _mm256_set1_epi16(0xFF)), _mm256_set1_epi16(128));
                    v00 = _mm256_sub_epi16(_mm256_srli_epi16(uv00, 8), _mm256_set1_epi16(128));
                }
            }
            else
            {
                u00 = _mm256_sub_epi16(_mm256_unpacklo_epi8(_mm256_castsi128_si256(_mm_load_si128((__m128i*)u0)), __m256i()), _mm256_set1_epi16(128)); u0 += 16;
                v00 = _mm256_sub_epi16(_mm256_unpacklo_epi8(_mm256_castsi128_si256(_mm_load_si128((__m128i*)v0)), __m256i()), _mm256_set1_epi16(128)); v0 += 16;
            }

            __m256i dR = _mm256_srai_epi16(                                                                                      _mm256_mullo_epi16(v00, _mm256_set1_epi16((short)( 1.28033 * 128))),  7);
            __m256i dG = _mm256_srai_epi16(_mm256_add_epi16(_mm256_mullo_epi16(u00, _mm256_set1_epi16((short)(-0.21482 * 256))), _mm256_mullo_epi16(v00, _mm256_set1_epi16((short)(-0.38059 * 256)))), 8);
            __m256i dB = _mm256_srai_epi16(                 _mm256_mullo_epi16(u00, _mm256_set1_epi16((short)( 2.12798 *  64))),                                                                       6);

            __m256i xR[2] = { _mm256_unpacklo_epi16(dR, dR), _mm256_unpackhi_epi16(dR, dR) };
            __m256i xG[2] = { _mm256_unpacklo_epi16(dG, dG), _mm256_unpackhi_epi16(dG, dG) };
            __m256i xB[2] = { _mm256_unpacklo_epi16(dB, dB), _mm256_unpackhi_epi16(dB, dB) };

            __m256i t[4];
            __m256i b[4];

            t[iR] = _mm256_packus_epi16(_mm256_add_epi16(y00, xR[0]), _mm256_add_epi16(y01, xR[1]));
            t[iG] = _mm256_packus_epi16(_mm256_add_epi16(y00, xG[0]), _mm256_add_epi16(y01, xG[1]));
            t[iB] = _mm256_packus_epi16(_mm256_add_epi16(y00, xB[0]), _mm256_add_epi16(y01, xB[1]));
            t[iA] = _mm256_set1_epi8(-1);
            b[iR] = _mm256_packus_epi16(_mm256_add_epi16(y10, xR[0]), _mm256_add_epi16(y11, xR[1]));
            b[iG] = _mm256_packus_epi16(_mm256_add_epi16(y10, xG[0]), _mm256_add_epi16(y11, xG[1]));
            b[iB] = _mm256_packus_epi16(_mm256_add_epi16(y10, xB[0]), _mm256_add_epi16(y11, xB[1]));
            b[iA] = _mm256_set1_epi8(-1);

            _MM256_TRANSPOSE4_EPI8(t[0], t[1], t[2], t[3]);
            _MM256_TRANSPOSE4_EPI8(b[0], b[1], b[2], b[3]);

            _mm256_storeu2_m128i((__m128i*)rgb0 + 4, (__m128i*)rgb0 + 0, t[0]);
            _mm256_storeu2_m128i((__m128i*)rgb0 + 5, (__m128i*)rgb0 + 1, t[1]);
            _mm256_storeu2_m128i((__m128i*)rgb0 + 6, (__m128i*)rgb0 + 2, t[2]);
            _mm256_storeu2_m128i((__m128i*)rgb0 + 7, (__m128i*)rgb0 + 3, t[3]); rgb0 += 16 * 8;
            _mm256_storeu2_m128i((__m128i*)rgb1 + 4, (__m128i*)rgb1 + 0, b[0]);
            _mm256_storeu2_m128i((__m128i*)rgb1 + 5, (__m128i*)rgb1 + 1, b[1]);
            _mm256_storeu2_m128i((__m128i*)rgb1 + 6, (__m128i*)rgb1 + 2, b[2]);
            _mm256_storeu2_m128i((__m128i*)rgb1 + 7, (__m128i*)rgb1 + 3, b[3]); rgb1 += 16 * 8;
        }
        if (rgbWidth == 4)
            continue;
#endif
        int halfWidth8 = (rgbWidth == 4) ? halfWidth / 8 : 0;
        for (int w = 0; w < halfWidth8; ++w)
        {
            __m128i y00lh = _mm_loadu_si128((__m128i*)y0);  y0 += 16;
            __m128i y10lh = _mm_loadu_si128((__m128i*)y1);  y1 += 16;
            __m128i y00 = _mm_unpacklo_epi8(y00lh, __m128i());
            __m128i y01 = _mm_unpackhi_epi8(y00lh, __m128i());
            __m128i y10 = _mm_unpacklo_epi8(y10lh, __m128i());
            __m128i y11 = _mm_unpackhi_epi8(y10lh, __m128i());

            __m128i u00;
            __m128i v00;
            if (interleaved)
            {
                if (firstV)
                {
                    __m128i uv00 = _mm_loadu_si128((__m128i*)v0);   v0 += 16;
                    u00 = _mm_sub_epi16(_mm_srli_epi16(uv00, 8), _mm_set1_epi16(128));
                    v00 = _mm_sub_epi16(_mm_and_si128(uv00, _mm_set1_epi16(0xFF)), _mm_set1_epi16(128));
                }
                else
                {
                    __m128i uv00 = _mm_loadu_si128((__m128i*)u0);   u0 += 16;
                    u00 = _mm_sub_epi16(_mm_and_si128(uv00, _mm_set1_epi16(0xFF)), _mm_set1_epi16(128));
                    v00 = _mm_sub_epi16(_mm_srli_epi16(uv00, 8), _mm_set1_epi16(128));
                }
            }
            else
            {
                u00 = _mm_sub_epi16(_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)u0), __m128i()), _mm_set1_epi16(128));  u0 += 8;
                v00 = _mm_sub_epi16(_mm_unpacklo_epi8(_mm_loadl_epi64((__m128i*)v0), __m128i()), _mm_set1_epi16(128));  v0 += 8;
            }

            __m128i dR = _mm_srai_epi16(                                                                             _mm_mullo_epi16(v00, _mm_set1_epi16((short)( 1.28033 * 128))),  7);
            __m128i dG = _mm_srai_epi16(_mm_add_epi16(_mm_mullo_epi16(u00, _mm_set1_epi16((short)(-0.21482 * 256))), _mm_mullo_epi16(v00, _mm_set1_epi16((short)(-0.38059 * 256)))), 8);
            __m128i dB = _mm_srai_epi16(              _mm_mullo_epi16(u00, _mm_set1_epi16((short)( 2.12798 *  64))),                                                                 6);

            __m128i xR[2] = { _mm_unpacklo_epi16(dR, dR), _mm_unpackhi_epi16(dR, dR) };
            __m128i xG[2] = { _mm_unpacklo_epi16(dG, dG), _mm_unpackhi_epi16(dG, dG) };
            __m128i xB[2] = { _mm_unpacklo_epi16(dB, dB), _mm_unpackhi_epi16(dB, dB) };

            __m128i t[4];
            __m128i b[4];

            t[iR] = _mm_packus_epi16(_mm_add_epi16(y00, xR[0]), _mm_add_epi16(y01, xR[1]));
            t[iG] = _mm_packus_epi16(_mm_add_epi16(y00, xG[0]), _mm_add_epi16(y01, xG[1]));
            t[iB] = _mm_packus_epi16(_mm_add_epi16(y00, xB[0]), _mm_add_epi16(y01, xB[1]));
            t[iA] = _mm_set1_epi8(-1);
            b[iR] = _mm_packus_epi16(_mm_add_epi16(y10, xR[0]), _mm_add_epi16(y11, xR[1]));
            b[iG] = _mm_packus_epi16(_mm_add_epi16(y10, xG[0]), _mm_add_epi16(y11, xG[1]));
            b[iB] = _mm_packus_epi16(_mm_add_epi16(y10, xB[0]), _mm_add_epi16(y11, xB[1]));
            b[iA] = _mm_set1_epi8(-1);

            _MM_TRANSPOSE4_EPI8(t[0], t[1], t[2], t[3]);
            _MM_TRANSPOSE4_EPI8(b[0], b[1], b[2], b[3]);

            _mm_storeu_si128((__m128i*)rgb0 + 0, t[0]);
            _mm_storeu_si128((__m128i*)rgb0 + 1, t[1]);
            _mm_storeu_si128((__m128i*)rgb0 + 2, t[2]);
            _mm_storeu_si128((__m128i*)rgb0 + 3, t[3]); rgb0 += 16 * 4;
            _mm_storeu_si128((__m128i*)rgb1 + 0, b[0]);
            _mm_storeu_si128((__m128i*)rgb1 + 1, b[1]);
            _mm_storeu_si128((__m128i*)rgb1 + 2, b[2]);
            _mm_storeu_si128((__m128i*)rgb1 + 3, b[3]); rgb1 += 16 * 4;
        }
        if (rgbWidth == 4)
            continue;
#endif
        for (int w = 0; w < halfWidth; ++w)
        {
            int y00 = (*y0++);
            int y01 = (*y0++);
            int y10 = (*y1++);
            int y11 = (*y1++);
            int u00 = (*u0++) - 128;
            int v00 = (*v0++) - 128;
            if (interleaved)
            {
                u0++;
                v0++;
            }

            // BT.709
            //     Y        U        V
            // R = 1  0.00000  1.28033
            // G = 1 -0.21482 -0.38059
            // B = 1  2.12798  0.00000
            int dR = (                              v00 * (int)( 1.28033 * 128)) >> 7;
            int dG = (u00 * (int)(-0.21482 * 256) + v00 * (int)(-0.38059 * 256)) >> 8;
            int dB = (u00 * (int)( 2.12798 *  64)                              ) >> 6;

            auto clamp = [](int value) -> unsigned char
            {
                return (unsigned char)(value < 255 ? value < 0 ? 0 : value : 255);
            };

            if (rgbWidth >= 1) rgb0[iR] = clamp(y00 + dR);
            if (rgbWidth >= 2) rgb0[iG] = clamp(y00 + dG);
            if (rgbWidth >= 3) rgb0[iB] = clamp(y00 + dB);
            if (rgbWidth >= 4) rgb0[iA] = 255;
            rgb0 += rgbWidth;

            if (rgbWidth >= 1) rgb0[iR] = clamp(y01 + dR);
            if (rgbWidth >= 2) rgb0[iG] = clamp(y01 + dG);
            if (rgbWidth >= 3) rgb0[iB] = clamp(y01 + dB);
            if (rgbWidth >= 4) rgb0[iA] = 255;
            rgb0 += rgbWidth;

            if (rgbWidth >= 1) rgb1[iR] = clamp(y10 + dR);
            if (rgbWidth >= 2) rgb1[iG] = clamp(y10 + dG);
            if (rgbWidth >= 3) rgb1[iB] = clamp(y10 + dB);
            if (rgbWidth >= 4) rgb1[iA] = 255;
            rgb1 += rgbWidth;

            if (rgbWidth >= 1) rgb1[iR] = clamp(y11 + dR);
            if (rgbWidth >= 2) rgb1[iG] = clamp(y11 + dG);
            if (rgbWidth >= 3) rgb1[iB] = clamp(y11 + dB);
            if (rgbWidth >= 4) rgb1[iA] = 255;
            rgb1 += rgbWidth;
        }
    }
}
//------------------------------------------------------------------------------
void yuv2rgb_yu12(int width, int height, const void* yuv, void* rgb, int rgbWidth, bool rgbSwizzle, int strideRGB, int alignWidth, int alignHeight)
{
    int sizeY = align(width, alignWidth) * align(height, alignHeight);
    int sizeUV = align(width / 2, alignWidth) * align(height / 2, alignHeight);

    if (strideRGB == 0)
        strideRGB = rgbWidth * width;

    if (rgbSwizzle)
    {
        if (rgbWidth == 3)
            yuv2rgb<3, true, false, false>(width, height, yuv, (char*)yuv + sizeY, (char*)yuv + sizeY + sizeUV, width, width / 2, width / 2, rgb, strideRGB);
        else if (rgbWidth == 4)
            yuv2rgb<4, true, false, false>(width, height, yuv, (char*)yuv + sizeY, (char*)yuv + sizeY + sizeUV, width, width / 2, width / 2, rgb, strideRGB);
    }
    else
    {
        if (rgbWidth == 3)
            yuv2rgb<3, false, false, false>(width, height, yuv, (char*)yuv + sizeY, (char*)yuv + sizeY + sizeUV, width, width / 2, width / 2, rgb, strideRGB);
        else if (rgbWidth == 4)
            yuv2rgb<4, false, false, false>(width, height, yuv, (char*)yuv + sizeY, (char*)yuv + sizeY + sizeUV, width, width / 2, width / 2, rgb, strideRGB);
    }
}
//------------------------------------------------------------------------------
void yuv2rgb_yv12(int width, int height, const void* yuv, void* rgb, int rgbWidth, bool rgbSwizzle, int strideRGB, int alignWidth, int alignHeight)
{
    int sizeY = align(width, alignWidth) * align(height, alignHeight);
    int sizeUV = align(width / 2, alignWidth) * align(height / 2, alignHeight);

    if (strideRGB == 0)
        strideRGB = rgbWidth * width;

    if (rgbSwizzle)
    {
        if (rgbWidth == 3)
            yuv2rgb<3, true, false, true>(width, height, yuv, (char*)yuv + sizeY + sizeUV, (char*)yuv + sizeY, width, width / 2, width / 2, rgb, strideRGB);
        else if (rgbWidth == 4)
            yuv2rgb<4, true, false, true>(width, height, yuv, (char*)yuv + sizeY + sizeUV, (char*)yuv + sizeY, width, width / 2, width / 2, rgb, strideRGB);
    }
    else
    {
        if (rgbWidth == 3)
            yuv2rgb<3, false, false, true>(width, height, yuv, (char*)yuv + sizeY + sizeUV, (char*)yuv + sizeY, width, width / 2, width / 2, rgb, strideRGB);
        else if (rgbWidth == 4)
            yuv2rgb<4, false, false, true>(width, height, yuv, (char*)yuv + sizeY + sizeUV, (char*)yuv + sizeY, width, width / 2, width / 2, rgb, strideRGB);
    }
}
//------------------------------------------------------------------------------
void yuv2rgb_nv12(int width, int height, const void* yuv, void* rgb, int rgbWidth, bool rgbSwizzle, int strideRGB, int alignWidth, int alignHeight)
{
    int sizeY = align(width, alignWidth) * align(height, alignHeight);

    if (strideRGB == 0)
        strideRGB = rgbWidth * width;

    if (rgbSwizzle)
    {
        if (rgbWidth == 3)
            yuv2rgb<3, true, true, false>(width, height, yuv, (char*)yuv + sizeY, (char*)yuv + sizeY + 1, width, width, width, rgb, strideRGB);
        else if (rgbWidth == 4)
            yuv2rgb<4, true, true, false>(width, height, yuv, (char*)yuv + sizeY, (char*)yuv + sizeY + 1, width, width, width, rgb, strideRGB);
    }
    else
    {
        if (rgbWidth == 3)
            yuv2rgb<3, false, true, false>(width, height, yuv, (char*)yuv + sizeY, (char*)yuv + sizeY + 1, width, width, width, rgb, strideRGB);
        else if (rgbWidth == 4)
            yuv2rgb<4, false, true, false>(width, height, yuv, (char*)yuv + sizeY, (char*)yuv + sizeY + 1, width, width, width, rgb, strideRGB);
    }
}
//------------------------------------------------------------------------------
void yuv2rgb_nv21(int width, int height, const void* yuv, void* rgb, int rgbWidth, bool rgbSwizzle, int strideRGB, int alignWidth, int alignHeight)
{
    int sizeY = align(width, alignWidth) * align(height, alignHeight);

    if (strideRGB == 0)
        strideRGB = rgbWidth * width;

    if (rgbSwizzle)
    {
        if (rgbWidth == 3)
            yuv2rgb<3, true, true, true>(width, height, yuv, (char*)yuv + sizeY + 1, (char*)yuv + sizeY, width, width, width, rgb, strideRGB);
        else if (rgbWidth == 4)
            yuv2rgb<4, true, true, true>(width, height, yuv, (char*)yuv + sizeY + 1, (char*)yuv + sizeY, width, width, width, rgb, strideRGB);
    }
    else
    {
        if (rgbWidth == 3)
            yuv2rgb<3, false, true, true>(width, height, yuv, (char*)yuv + sizeY + 1, (char*)yuv + sizeY, width, width, width, rgb, strideRGB);
        else if (rgbWidth == 4)
            yuv2rgb<4, false, true, true>(width, height, yuv, (char*)yuv + sizeY + 1, (char*)yuv + sizeY, width, width, width, rgb, strideRGB);
    }
}
//------------------------------------------------------------------------------
