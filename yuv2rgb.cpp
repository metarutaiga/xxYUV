//==============================================================================
// xxYUV : yuv2rgb Source
//
// Copyright (c) 2020 TAiGA
// https://github.com/metarutaiga/xxYUV
//==============================================================================
#if defined(__ARM_NEON__) || defined(__ARM_NEON) || defined(_M_ARM) || defined(_M_ARM64)
#   include <arm_neon.h>
#   define NEON_FAST 1
#elif defined(__AVX512F__) && defined(__AVX512BW__) && 0
#   include <immintrin.h>
#   define _MM512_TRANSPOSE4_EPI8(R0, R1, R2, R3) { \
        __m512i T0, T1, T2, T3;                     \
        T0 = _mm512_unpacklo_epi8(R0, R1);          \
        T1 = _mm512_unpacklo_epi8(R2, R3);          \
        T2 = _mm512_unpackhi_epi8(R0, R1);          \
        T3 = _mm512_unpackhi_epi8(R2, R3);          \
        R0 = _mm512_unpacklo_epi16(T0, T1);         \
        R1 = _mm512_unpackhi_epi16(T0, T1);         \
        R2 = _mm512_unpacklo_epi16(T2, T3);         \
        R3 = _mm512_unpackhi_epi16(T2, T3);         \
    }
#elif defined(__AVX2__)
#   include <immintrin.h>
#   define _MM256_TRANSPOSE4_EPI8(R0, R1, R2, R3) { \
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
#endif
#include "yuv2rgb.h"

#define align(v, a) ((v) + ((a) - 1) & ~((a) - 1))

// BT.709 - Video Range
//     Y         U         V
// R = 1.164384  0.000000  1.792741
// G = 1.164384 -0.213249 -0.532909
// B = 1.164384  2.112402  0.000000
//
// BT.709 - Full Range
//     Y         U         V
// R = 1.000000  0.000000  1.581000
// G = 1.000000 -0.188062 -0.469967
// B = 1.000000  1.862906  0.000000
#define vY   1.164384
#define vUG -0.213249
#define vUB  2.112402
#define vVR  1.792741
#define vVG -0.532909
#define fY   1.000000
#define fUG -0.188062
#define fUB  1.862906
#define fVR  1.581000
#define fVG -0.469967

//------------------------------------------------------------------------------
template<int rgbWidth, bool rgbSwizzle, bool interleaved, bool firstU, bool fullRange>
void yuv2rgb(int width, int height, const void* y, const void* u, const void* v, int strideY, int strideU, int strideV, void* rgb, int strideRGB)
{
    int halfWidth = width >> 1;
    int halfHeight = height >> 1;

    int iR = rgbSwizzle ? 2 : 0;
    int iG = 1;
    int iB = rgbSwizzle ? 0 : 2;
    int iA = 3;

    int Y, UG, UB, VR, VG;
    if (fullRange)
    {
        Y = (int)(fY * 256);
        UG = (int)(fUG * 256); UB = (int)(fUB * 256);
        VR = (int)(fVR * 256); VG = (int)(fVG * 256);
    }
    else
    {
        Y = (int)(vY * 256);
        UG = (int)(vUG * 256); UB = (int)(vUB * 256);
        VR = (int)(vVR * 256); VG = (int)(vVG * 256);
    }

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
            if (fullRange)
            {
            }
            else
            {
                y00lh = vqsubq_u8(y00lh, vdupq_n_u8(16));
                y10lh = vqsubq_u8(y10lh, vdupq_n_u8(16));
                y00 = vshrn_n_u16(vmull_u8(vget_low_u8(y00lh), vdup_n_u8(Y / 2)), 7);
                y01 = vshrn_n_u16(vmull_u8(vget_high_u8(y00lh), vdup_n_u8(Y / 2)), 7);
                y10 = vshrn_n_u16(vmull_u8(vget_low_u8(y10lh), vdup_n_u8(Y / 2)), 7);
                y11 = vshrn_n_u16(vmull_u8(vget_high_u8(y10lh), vdup_n_u8(Y / 2)), 7);
            }

            int8x8_t u000;
            int8x8_t v000;
            if (interleaved)
            {
                if (firstU)
                {
                    int8x16_t uv00 = vld1q_u8(u0); u0 += 16;
                    int8x8x2_t uv00lh = vuzp_s8(vget_low_s8(uv00), vget_high_s8(uv00));
                    int8x16_t uv000 = vaddq_s8(vcombine_s8(uv00lh.val[0], uv00lh.val[1]), vdupq_n_s8(-128));
                    u000 = vget_low_s8(uv000);
                    v000 = vget_high_s8(uv000);
                }
                else
                {
                    int8x16_t uv00 = vld1q_u8(v0); v0 += 16;
                    int8x8x2_t uv00lh = vuzp_s8(vget_low_s8(uv00), vget_high_s8(uv00));
                    int8x16_t uv000 = vaddq_s8(vcombine_s8(uv00lh.val[1], uv00lh.val[0]), vdupq_n_s8(-128));
                    u000 = vget_low_s8(uv000);
                    v000 = vget_high_s8(uv000);
                }
            }
            else
            {
                int8x16_t uv000 = vaddq_s8(vcombine_s8(vld1_u8(u0), vld1_u8(v0)), vdupq_n_s8(-128)); u0 += 8; v0 += 8;
                u000 = vget_low_s8(uv000);
                v000 = vget_high_s8(uv000);
            }

#if NEON_FAST
            int16x8_t dR = vshrq_n_s16(                                   vmull_s8(v000, vdup_n_s8(VR / 4)), 6);
            int16x8_t dG = vshrq_n_s16(vmlal_s8(vmull_s8(u000, vdup_n_s8(UG / 2)), v000, vdup_n_s8(VG / 2)), 7);
            int16x8_t dB = vshrq_n_s16(         vmull_s8(u000, vdup_n_s8(UB / 8)),                           5);
#else
            int16x8_t u00 = vshll_n_s8(u000, 7);
            int16x8_t v00 = vshll_n_s8(v000, 7);

            int16x8_t dR =                                               vqdmulhq_s16(v00, vdupq_n_s16(VR));
            int16x8_t dG = vaddq_s16(vqdmulhq_s16(u00, vdupq_n_s16(UG)), vqdmulhq_s16(v00, vdupq_n_s16(VG)));
            int16x8_t dB =           vqdmulhq_s16(u00, vdupq_n_s16(UB));
#endif

            uint16x8x2_t xR = vzipq_u16(vreinterpretq_u16_s16(dR), vreinterpretq_u16_s16(dR));
            uint16x8x2_t xG = vzipq_u16(vreinterpretq_u16_s16(dG), vreinterpretq_u16_s16(dG));
            uint16x8x2_t xB = vzipq_u16(vreinterpretq_u16_s16(dB), vreinterpretq_u16_s16(dB));

            uint8x16x4_t t;
            uint8x16x4_t b;

            t.val[iR] = vcombine_u8(vqmovun_s16(vaddw_u8(xR.val[0], y00)), vqmovun_s16(vaddw_u8(xR.val[1], y01)));
            t.val[iG] = vcombine_u8(vqmovun_s16(vaddw_u8(xG.val[0], y00)), vqmovun_s16(vaddw_u8(xG.val[1], y01)));
            t.val[iB] = vcombine_u8(vqmovun_s16(vaddw_u8(xB.val[0], y00)), vqmovun_s16(vaddw_u8(xB.val[1], y01)));
            t.val[iA] = vdupq_n_u8(255);
            b.val[iR] = vcombine_u8(vqmovun_s16(vaddw_u8(xR.val[0], y10)), vqmovun_s16(vaddw_u8(xR.val[1], y11)));
            b.val[iG] = vcombine_u8(vqmovun_s16(vaddw_u8(xG.val[0], y10)), vqmovun_s16(vaddw_u8(xG.val[1], y11)));
            b.val[iB] = vcombine_u8(vqmovun_s16(vaddw_u8(xB.val[0], y10)), vqmovun_s16(vaddw_u8(xB.val[1], y11)));
            b.val[iA] = vdupq_n_u8(255);

            vst4q_u8(rgb0, t);  rgb0 += 16 * 4;
            vst4q_u8(rgb1, b);  rgb1 += 16 * 4;
        }
        if (rgbWidth == 4)
            continue;
#elif defined(__AVX512F__) && defined(__AVX512BW__) && 0
        int halfWidth16 = (rgbWidth == 4) ? halfWidth / 32 : 0;
        for (int w = 0; w < halfWidth16; ++w)
        {
            __m512i y00lh = _mm512_loadu_si512((__m512i*)y0); y0 += 64;
            __m512i y10lh = _mm512_loadu_si512((__m512i*)y1); y1 += 64;
            __m512i y00;
            __m512i y01;
            __m512i y10;
            __m512i y11;
            if (fullRange)
            {
                y00 = _mm512_unpacklo_epi8(y00lh, __m512i());
                y01 = _mm512_unpackhi_epi8(y00lh, __m512i());
                y10 = _mm512_unpacklo_epi8(y10lh, __m512i());
                y11 = _mm512_unpackhi_epi8(y10lh, __m512i());
            }
            else
            {
                y00lh = _mm512_subs_epu8(y00lh, _mm512_set1_epi8(16));
                y10lh = _mm512_subs_epu8(y10lh, _mm512_set1_epi8(16));
                y00 = _mm512_mulhi_epu16(_mm512_unpacklo_epi8(__m512i(), y00lh), _mm512_set1_epi16(Y));
                y01 = _mm512_mulhi_epu16(_mm512_unpackhi_epi8(__m512i(), y00lh), _mm512_set1_epi16(Y));
                y10 = _mm512_mulhi_epu16(_mm512_unpacklo_epi8(__m512i(), y10lh), _mm512_set1_epi16(Y));
                y11 = _mm512_mulhi_epu16(_mm512_unpackhi_epi8(__m512i(), y10lh), _mm512_set1_epi16(Y));
            }

            __m512i u00;
            __m512i v00;
            if (interleaved)
            {
                if (firstU)
                {
                    __m512i uv00 = _mm512_loadu_si512((__m512i*)u0); u0 += 64;
                    u00 = _mm512_add_epi16(_mm512_slli_epi16(uv00, 8), _mm512_set1_epi16(-32768));
                    v00 = _mm512_add_epi16(_mm512_and_si512(uv00, _mm512_set1_epi16(0xFF00)), _mm512_set1_epi16(-32768));
                }
                else
                {
                    __m512i uv00 = _mm512_loadu_si512((__m512i*)v0); v0 += 64;
                    u00 = _mm512_add_epi16(_mm512_and_si512(uv00, _mm512_set1_epi16(0xFF00)), _mm512_set1_epi16(-32768));
                    v00 = _mm512_add_epi16(_mm512_slli_epi16(uv00, 8), _mm512_set1_epi16(-32768));
                }
            }
            else
            {
                u00 = _mm512_add_epi16(_mm512_slli_epi16(_mm512_cvtepu8_epi16(_mm256_loadu_si256((__m256i*)u0)), 8), _mm512_set1_epi16(-32768)); u0 += 32;
                v00 = _mm512_add_epi16(_mm512_slli_epi16(_mm512_cvtepu8_epi16(_mm256_loadu_si256((__m256i*)v0)), 8), _mm512_set1_epi16(-32768)); v0 += 32;
            }

            __m512i dR =                                                                  _mm512_mulhi_epi16(v00, _mm512_set1_epi16(VR));
            __m512i dG = _mm512_add_epi16(_mm512_mulhi_epi16(u00, _mm512_set1_epi16(UG)), _mm512_mulhi_epi16(v00, _mm512_set1_epi16(VG)));
            __m512i dB =                  _mm512_mulhi_epi16(u00, _mm512_set1_epi16(UB));

            __m512i xR[2] = { _mm512_unpacklo_epi16(dR, dR), _mm512_unpackhi_epi16(dR, dR) };
            __m512i xG[2] = { _mm512_unpacklo_epi16(dG, dG), _mm512_unpackhi_epi16(dG, dG) };
            __m512i xB[2] = { _mm512_unpacklo_epi16(dB, dB), _mm512_unpackhi_epi16(dB, dB) };

            __m512i t[4];
            __m512i b[4];

            t[iR] = _mm512_packus_epi16(_mm512_add_epi16(y00, xR[0]), _mm512_add_epi16(y01, xR[1]));
            t[iG] = _mm512_packus_epi16(_mm512_add_epi16(y00, xG[0]), _mm512_add_epi16(y01, xG[1]));
            t[iB] = _mm512_packus_epi16(_mm512_add_epi16(y00, xB[0]), _mm512_add_epi16(y01, xB[1]));
            t[iA] = _mm512_set1_epi8(-1);
            b[iR] = _mm512_packus_epi16(_mm512_add_epi16(y10, xR[0]), _mm512_add_epi16(y11, xR[1]));
            b[iG] = _mm512_packus_epi16(_mm512_add_epi16(y10, xG[0]), _mm512_add_epi16(y11, xG[1]));
            b[iB] = _mm512_packus_epi16(_mm512_add_epi16(y10, xB[0]), _mm512_add_epi16(y11, xB[1]));
            b[iA] = _mm512_set1_epi8(-1);

            _MM512_TRANSPOSE4_EPI8(t[0], t[1], t[2], t[3]);
            _MM512_TRANSPOSE4_EPI8(b[0], b[1], b[2], b[3]);

            _mm512_mask_storeu_epi32((__m128i*)rgb0 +  0, 0x000F, t[0]);
            _mm512_mask_storeu_epi32((__m128i*)rgb0 +  1, 0x00F0, t[0]);
            _mm512_mask_storeu_epi32((__m128i*)rgb0 +  2, 0x0F00, t[0]);
            _mm512_mask_storeu_epi32((__m128i*)rgb0 +  3, 0xF000, t[0]);
            _mm512_mask_storeu_epi32((__m128i*)rgb0 +  4, 0x000F, t[1]);
            _mm512_mask_storeu_epi32((__m128i*)rgb0 +  5, 0x00F0, t[1]);
            _mm512_mask_storeu_epi32((__m128i*)rgb0 +  6, 0x0F00, t[1]);
            _mm512_mask_storeu_epi32((__m128i*)rgb0 +  7, 0xF000, t[1]);
            _mm512_mask_storeu_epi32((__m128i*)rgb0 +  8, 0x000F, t[2]);
            _mm512_mask_storeu_epi32((__m128i*)rgb0 +  9, 0x00F0, t[2]);
            _mm512_mask_storeu_epi32((__m128i*)rgb0 + 10, 0x0F00, t[2]);
            _mm512_mask_storeu_epi32((__m128i*)rgb0 + 11, 0xF000, t[2]);
            _mm512_mask_storeu_epi32((__m128i*)rgb0 + 12, 0x000F, t[3]);
            _mm512_mask_storeu_epi32((__m128i*)rgb0 + 13, 0x00F0, t[3]);
            _mm512_mask_storeu_epi32((__m128i*)rgb0 + 14, 0x0F00, t[3]);
            _mm512_mask_storeu_epi32((__m128i*)rgb0 + 15, 0xF000, t[3]);    rgb0 += 16 * 16;
            _mm512_mask_storeu_epi32((__m128i*)rgb1 +  0, 0x000F, b[0]);
            _mm512_mask_storeu_epi32((__m128i*)rgb1 +  1, 0x00F0, b[0]);
            _mm512_mask_storeu_epi32((__m128i*)rgb1 +  2, 0x0F00, b[0]);
            _mm512_mask_storeu_epi32((__m128i*)rgb1 +  3, 0xF000, b[0]);
            _mm512_mask_storeu_epi32((__m128i*)rgb1 +  4, 0x000F, b[1]);
            _mm512_mask_storeu_epi32((__m128i*)rgb1 +  5, 0x00F0, b[1]);
            _mm512_mask_storeu_epi32((__m128i*)rgb1 +  6, 0x0F00, b[1]);
            _mm512_mask_storeu_epi32((__m128i*)rgb1 +  7, 0xF000, b[1]);
            _mm512_mask_storeu_epi32((__m128i*)rgb1 +  8, 0x000F, b[2]);
            _mm512_mask_storeu_epi32((__m128i*)rgb1 +  9, 0x00F0, b[2]);
            _mm512_mask_storeu_epi32((__m128i*)rgb1 + 10, 0x0F00, b[2]);
            _mm512_mask_storeu_epi32((__m128i*)rgb1 + 11, 0xF000, b[2]);
            _mm512_mask_storeu_epi32((__m128i*)rgb1 + 12, 0x000F, b[3]);
            _mm512_mask_storeu_epi32((__m128i*)rgb1 + 13, 0x00F0, b[3]);
            _mm512_mask_storeu_epi32((__m128i*)rgb1 + 14, 0x0F00, b[3]);
            _mm512_mask_storeu_epi32((__m128i*)rgb1 + 15, 0xF000, b[3]);    rgb1 += 16 * 16;
        }
        if (rgbWidth == 4)
            continue;
#elif defined(__AVX2__)
        int halfWidth16 = (rgbWidth == 4) ? halfWidth / 16 : 0;
        for (int w = 0; w < halfWidth16; ++w)
        {
            __m256i y00lh = _mm256_loadu_si256((__m256i*)y0); y0 += 32;
            __m256i y10lh = _mm256_loadu_si256((__m256i*)y1); y1 += 32;
            __m256i y00;
            __m256i y01;
            __m256i y10;
            __m256i y11;
            if (fullRange)
            {
                y00 = _mm256_unpacklo_epi8(y00lh, __m256i());
                y01 = _mm256_unpackhi_epi8(y00lh, __m256i());
                y10 = _mm256_unpacklo_epi8(y10lh, __m256i());
                y11 = _mm256_unpackhi_epi8(y10lh, __m256i());
            }
            else
            {
                y00lh = _mm256_subs_epu8(y00lh, _mm256_set1_epi8(16));
                y10lh = _mm256_subs_epu8(y10lh, _mm256_set1_epi8(16));
                y00 = _mm256_mulhi_epu16(_mm256_unpacklo_epi8(__m256i(), y00lh), _mm256_set1_epi16(Y));
                y01 = _mm256_mulhi_epu16(_mm256_unpackhi_epi8(__m256i(), y00lh), _mm256_set1_epi16(Y));
                y10 = _mm256_mulhi_epu16(_mm256_unpacklo_epi8(__m256i(), y10lh), _mm256_set1_epi16(Y));
                y11 = _mm256_mulhi_epu16(_mm256_unpackhi_epi8(__m256i(), y10lh), _mm256_set1_epi16(Y));
            }

            __m256i u00;
            __m256i v00;
            if (interleaved)
            {
                if (firstU)
                {
                    __m256i uv00 = _mm256_loadu_si256((__m256i*)u0); u0 += 32;
                    u00 = _mm256_add_epi16(_mm256_slli_epi16(uv00, 8), _mm256_set1_epi16(-32768));
                    v00 = _mm256_add_epi16(_mm256_and_si256(uv00, _mm256_set1_epi16(0xFF00)), _mm256_set1_epi16(-32768));
                }
                else
                {
                    __m256i uv00 = _mm256_loadu_si256((__m256i*)v0); v0 += 32;
                    u00 = _mm256_add_epi16(_mm256_and_si256(uv00, _mm256_set1_epi16(0xFF00)), _mm256_set1_epi16(-32768));
                    v00 = _mm256_add_epi16(_mm256_slli_epi16(uv00, 8), _mm256_set1_epi16(-32768));
                }
            }
            else
            {
                u00 = _mm256_add_epi16(_mm256_slli_epi16(_mm256_cvtepu8_epi16(_mm_loadu_si128((__m128i*)u0)), 8), _mm256_set1_epi16(-32768)); u0 += 16;
                v00 = _mm256_add_epi16(_mm256_slli_epi16(_mm256_cvtepu8_epi16(_mm_loadu_si128((__m128i*)v0)), 8), _mm256_set1_epi16(-32768)); v0 += 16;
            }

            __m256i dR =                                                                  _mm256_mulhi_epi16(v00, _mm256_set1_epi16(VR));
            __m256i dG = _mm256_add_epi16(_mm256_mulhi_epi16(u00, _mm256_set1_epi16(UG)), _mm256_mulhi_epi16(v00, _mm256_set1_epi16(VG)));
            __m256i dB =                  _mm256_mulhi_epi16(u00, _mm256_set1_epi16(UB));

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
#elif defined(_M_IX86) || defined(_M_AMD64) || defined(__i386__) || defined(__amd64__)
        int halfWidth8 = (rgbWidth == 4) ? halfWidth / 8 : 0;
        for (int w = 0; w < halfWidth8; ++w)
        {
            __m128i y00lh = _mm_loadu_si128((__m128i*)y0); y0 += 16;
            __m128i y10lh = _mm_loadu_si128((__m128i*)y1); y1 += 16;
            __m128i y00;
            __m128i y01;
            __m128i y10;
            __m128i y11;
            if (fullRange)
            {
                y00 = _mm_unpacklo_epi8(y00lh, __m128i());
                y01 = _mm_unpackhi_epi8(y00lh, __m128i());
                y10 = _mm_unpacklo_epi8(y10lh, __m128i());
                y11 = _mm_unpackhi_epi8(y10lh, __m128i());
            }
            else
            {
                y00lh = _mm_subs_epu8(y00lh, _mm_set1_epi8(16));
                y10lh = _mm_subs_epu8(y10lh, _mm_set1_epi8(16));
                y00 = _mm_mulhi_epu16(_mm_unpacklo_epi8(__m128i(), y00lh), _mm_set1_epi16(Y));
                y01 = _mm_mulhi_epu16(_mm_unpackhi_epi8(__m128i(), y00lh), _mm_set1_epi16(Y));
                y10 = _mm_mulhi_epu16(_mm_unpacklo_epi8(__m128i(), y10lh), _mm_set1_epi16(Y));
                y11 = _mm_mulhi_epu16(_mm_unpackhi_epi8(__m128i(), y10lh), _mm_set1_epi16(Y));
            }

            __m128i u00;
            __m128i v00;
            if (interleaved)
            {
                if (firstU)
                {
                    __m128i uv00 = _mm_loadu_si128((__m128i*)u0); u0 += 16;
                    u00 = _mm_add_epi16(_mm_slli_epi16(uv00, 8), _mm_set1_epi16(-32768));
                    v00 = _mm_add_epi16(_mm_and_si128(uv00, _mm_set1_epi16(0xFF00)), _mm_set1_epi16(-32768));
                }
                else
                {
                    __m128i uv00 = _mm_loadu_si128((__m128i*)v0); v0 += 16;
                    u00 = _mm_add_epi16(_mm_and_si128(uv00, _mm_set1_epi16(0xFF00)), _mm_set1_epi16(-32768));
                    v00 = _mm_add_epi16(_mm_slli_epi16(uv00, 8), _mm_set1_epi16(-32768));
                }
            }
            else
            {
                u00 = _mm_add_epi16(_mm_unpacklo_epi8(__m128i(), _mm_loadl_epi64((__m128i*)u0)), _mm_set1_epi16(-32768)); u0 += 8;
                v00 = _mm_add_epi16(_mm_unpacklo_epi8(__m128i(), _mm_loadl_epi64((__m128i*)v0)), _mm_set1_epi16(-32768)); v0 += 8;
            }

            __m128i dR =                                                         _mm_mulhi_epi16(v00, _mm_set1_epi16(VR));
            __m128i dG = _mm_add_epi16(_mm_mulhi_epi16(u00, _mm_set1_epi16(UG)), _mm_mulhi_epi16(v00, _mm_set1_epi16(VG)));
            __m128i dB =               _mm_mulhi_epi16(u00, _mm_set1_epi16(UB));

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
            if (fullRange)
            {
            }
            else
            {
                y00 = ((y00 - 16) * Y) >> 8;
                y01 = ((y01 - 16) * Y) >> 8;
                y10 = ((y10 - 16) * Y) >> 8;
                y11 = ((y11 - 16) * Y) >> 8;
            }

            int u00 = (*u0++) - 128;
            int v00 = (*v0++) - 128;
            if (interleaved)
            {
                u0++;
                v0++;
            }

            int dR = (           v00 * VR) >> 8;
            int dG = (u00 * UG + v00 * VG) >> 8;
            int dB = (u00 * UB           ) >> 8;

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
void yuv2rgb_yu12(int width, int height, const void* yuv, void* rgb, bool fullRange, int rgbWidth, bool rgbSwizzle, int strideRGB, int alignWidth, int alignHeight)
{
    int sizeY = align(width, alignWidth) * align(height, alignHeight);
    int sizeUV = align(width / 2, alignWidth) * align(height / 2, alignHeight);

    if (strideRGB == 0)
        strideRGB = rgbWidth * width;

    auto converter = yuv2rgb<3, false, false, false, false>;

    if (rgbWidth == 3)
    {
        if (rgbSwizzle)
        {
            if (fullRange)
                converter = yuv2rgb<3, true, false, false, true>;
            else
                converter = yuv2rgb<3, true, false, false, false>;
        }
        else
        {
            if (fullRange)
                converter = yuv2rgb<3, false, false, false, true>;
            else
                converter = yuv2rgb<3, false, false, false, false>;
        }
    }
    else if (rgbWidth == 4)
    {
        if (rgbSwizzle)
        {
            if (fullRange)
                converter = yuv2rgb<4, true, false, false, true>;
            else
                converter = yuv2rgb<4, true, false, false, false>;
        }
        else
        {
            if (fullRange)
                converter = yuv2rgb<4, false, false, false, true>;
            else
                converter = yuv2rgb<4, false, false, false, false>;
        }
    }

    converter(width, height, yuv, (char*)yuv + sizeY, (char*)yuv + sizeY + sizeUV, width, width / 2, width / 2, rgb, strideRGB);
}
//------------------------------------------------------------------------------
void yuv2rgb_yv12(int width, int height, const void* yuv, void* rgb, bool fullRange, int rgbWidth, bool rgbSwizzle, int strideRGB, int alignWidth, int alignHeight)
{
    int sizeY = align(width, alignWidth) * align(height, alignHeight);
    int sizeUV = align(width / 2, alignWidth) * align(height / 2, alignHeight);

    if (strideRGB == 0)
        strideRGB = rgbWidth * width;

    auto converter = yuv2rgb<3, false, false, false, false>;

    if (rgbWidth == 3)
    {
        if (rgbSwizzle)
        {
            if (fullRange)
                converter = yuv2rgb<3, true, false, false, true>;
            else
                converter = yuv2rgb<3, true, false, false, false>;
        }
        else
        {
            if (fullRange)
                converter = yuv2rgb<3, false, false, false, true>;
            else
                converter = yuv2rgb<3, false, false, false, false>;
        }
    }
    else if (rgbWidth == 4)
    {
        if (rgbSwizzle)
        {
            if (fullRange)
                converter = yuv2rgb<4, true, false, false, true>;
            else
                converter = yuv2rgb<4, true, false, false, false>;
        }
        else
        {
            if (fullRange)
                converter = yuv2rgb<4, false, false, false, true>;
            else
                converter = yuv2rgb<4, false, false, false, false>;
        }
    }

    converter(width, height, yuv, (char*)yuv + sizeY + sizeUV, (char*)yuv + sizeY, width, width / 2, width / 2, rgb, strideRGB);
}
//------------------------------------------------------------------------------
void yuv2rgb_nv12(int width, int height, const void* yuv, void* rgb, bool fullRange, int rgbWidth, bool rgbSwizzle, int strideRGB, int alignWidth, int alignHeight)
{
    int sizeY = align(width, alignWidth) * align(height, alignHeight);

    if (strideRGB == 0)
        strideRGB = rgbWidth * width;

    auto converter = yuv2rgb<3, false, false, false, false>;

    if (rgbWidth == 3)
    {
        if (rgbSwizzle)
        {
            if (fullRange)
                converter = yuv2rgb<3, true, true, true, true>;
            else
                converter = yuv2rgb<3, true, true, true, false>;
        }
        else
        {
            if (fullRange)
                converter = yuv2rgb<3, false, true, true, true>;
            else
                converter = yuv2rgb<3, false, true, true, false>;
        }
    }
    else if (rgbWidth == 4)
    {
        if (rgbSwizzle)
        {
            if (fullRange)
                converter = yuv2rgb<4, true, true, true, true>;
            else
                converter = yuv2rgb<4, true, true, true, false>;
        }
        else
        {
            if (fullRange)
                converter = yuv2rgb<4, false, true, true, true>;
            else
                converter = yuv2rgb<4, false, true, true, false>;
        }
    }

    converter(width, height, yuv, (char*)yuv + sizeY, (char*)yuv + sizeY + 1, width, width, width, rgb, strideRGB);
}
//------------------------------------------------------------------------------
void yuv2rgb_nv21(int width, int height, const void* yuv, void* rgb, bool fullRange, int rgbWidth, bool rgbSwizzle, int strideRGB, int alignWidth, int alignHeight)
{
    int sizeY = align(width, alignWidth) * align(height, alignHeight);

    if (strideRGB == 0)
        strideRGB = rgbWidth * width;

    auto converter = yuv2rgb<3, false, false, false, false>;

    if (rgbWidth == 3)
    {
        if (rgbSwizzle)
        {
            if (fullRange)
                converter = yuv2rgb<3, true, true, false, true>;
            else
                converter = yuv2rgb<3, true, true, false, false>;
        }
        else
        {
            if (fullRange)
                converter = yuv2rgb<3, false, true, false, true>;
            else
                converter = yuv2rgb<3, false, true, false, false>;
        }
    }
    else if (rgbWidth == 4)
    {
        if (rgbSwizzle)
        {
            if (fullRange)
                converter = yuv2rgb<4, true, true, false, true>;
            else
                converter = yuv2rgb<4, true, true, false, false>;
        }
        else
        {
            if (fullRange)
                converter = yuv2rgb<4, false, true, false, true>;
            else
                converter = yuv2rgb<4, false, true, false, false>;
        }
    }

    converter(width, height, yuv, (char*)yuv + sizeY + 1, (char*)yuv + sizeY, width, width, width, rgb, strideRGB);
}
//------------------------------------------------------------------------------
