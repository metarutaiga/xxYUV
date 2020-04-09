//==============================================================================
// xxYUV : yuv2rgb Source
//
// Copyright (c) 2020 TAiGA
// https://github.com/metarutaiga/xxYUV
//==============================================================================
#if defined(__ARM_NEON__) || defined(__ARM_NEON) || defined(_M_ARM) || defined(_M_ARM64)
#include <arm_neon.h>
#elif defined(_M_IX86) || defined(_M_AMD64) || defined(__i386__) || defined(__amd64__)
#include <emmintrin.h>
#define _MM_TRANSPOSE4_EPI8(R0, R1, R2, R3) {   \
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

//------------------------------------------------------------------------------
template<int rgbWidth, bool interleaved, bool firstV>
void yuv2rgb(int width, int height, const void* y, const void* u, const void* v, int strideY, int strideU, int strideV, void* rgb, int strideRGB)
{
    int halfWidth = width >> 1;
    int halfHeight = height >> 1;

    for (int h = 0; h < halfHeight; ++h)
    {
        const unsigned char* y0 = (unsigned char*)y;
        const unsigned char* y1 = y0 + strideY;         y = y1 + strideY;
        const unsigned char* u0 = (unsigned char*)u;    u = u0 + strideU;
        const unsigned char* v0 = (unsigned char*)v;    v = v0 + strideV;
        unsigned char* rgb0 = (unsigned char*)rgb;
        unsigned char* rgb1 = rgb0 + strideRGB;         rgb = rgb1 + strideRGB;
#if defined(__ARM_NEON__) || defined(__ARM_NEON) || defined(_M_ARM) || defined(_M_ARM64)
        for (int w = 0; w < halfWidth; w += 8)
        {
            uint8x16_t y00lh = vld1q_u8(y0); y0 += 16;
            uint8x16_t y10lh = vld1q_u8(y1); y1 += 16;
            int16x8_t y00 = vreinterpretq_s16_u16(vshll_n_u8(vget_low_u8(y00lh), 7));
            int16x8_t y01 = vreinterpretq_s16_u16(vshll_n_u8(vget_high_u8(y00lh), 7));
            int16x8_t y10 = vreinterpretq_s16_u16(vshll_n_u8(vget_low_u8(y10lh), 7));
            int16x8_t y11 = vreinterpretq_s16_u16(vshll_n_u8(vget_high_u8(y10lh), 7));

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

            int16x8_t dR =                                                      vmulq_n_s16(v00, (short)( 1.28033 * 128));
            int16x8_t dG = vaddq_s16(vmulq_n_s16(u00, (short)(-0.21482 * 128)), vmulq_n_s16(v00, (short)(-0.38059 * 128)));
            int16x8_t dB =           vmulq_n_s16(u00, (short)( 2.12798 * 128));

            int16x8x2_t xR = vzipq_s16(dR, dR);
            int16x8x2_t xG = vzipq_s16(dG, dG);
            int16x8x2_t xB = vzipq_s16(dB, dB);

            uint8x16x4_t t;
            uint8x16x4_t b;

            t.val[0] = vcombine_u8(vqshrun_n_s16(vqaddq_s16(y00, xR.val[0]), 7), vqshrun_n_s16(vqaddq_s16(y01, xR.val[1]), 7));
            t.val[1] = vcombine_u8(vqshrun_n_s16(vqaddq_s16(y00, xG.val[0]), 7), vqshrun_n_s16(vqaddq_s16(y01, xG.val[1]), 7));
            t.val[2] = vcombine_u8(vqshrun_n_s16(vqaddq_s16(y00, xB.val[0]), 7), vqshrun_n_s16(vqaddq_s16(y01, xB.val[1]), 7));
            t.val[3] = vdupq_n_u8(255);
            b.val[0] = vcombine_u8(vqshrun_n_s16(vqaddq_s16(y10, xR.val[0]), 7), vqshrun_n_s16(vqaddq_s16(y11, xR.val[1]), 7));
            b.val[1] = vcombine_u8(vqshrun_n_s16(vqaddq_s16(y10, xG.val[0]), 7), vqshrun_n_s16(vqaddq_s16(y11, xG.val[1]), 7));
            b.val[2] = vcombine_u8(vqshrun_n_s16(vqaddq_s16(y10, xB.val[0]), 7), vqshrun_n_s16(vqaddq_s16(y11, xB.val[1]), 7));
            b.val[3] = vdupq_n_u8(255);

            vst4q_u8(rgb0, t);  rgb0 += 16 * 4;
            vst4q_u8(rgb1, b);  rgb1 += 16 * 4;
        }
        continue;
#elif defined(_M_IX86) || defined(_M_AMD64) || defined(__i386__) || defined(__amd64__)
        for (int w = 0; w < halfWidth; w += 8)
        {
            __m128i y00lh = _mm_loadu_si128((__m128i*)y0);  y0 += 16;
            __m128i y10lh = _mm_loadu_si128((__m128i*)y1);  y1 += 16;
            __m128i y00 = _mm_slli_epi16(_mm_unpacklo_epi8(y00lh, __m128i()), 7);
            __m128i y01 = _mm_slli_epi16(_mm_unpackhi_epi8(y00lh, __m128i()), 7);
            __m128i y10 = _mm_slli_epi16(_mm_unpacklo_epi8(y10lh, __m128i()), 7);
            __m128i y11 = _mm_slli_epi16(_mm_unpackhi_epi8(y10lh, __m128i()), 7);

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

            __m128i dR =                                                                              _mm_mullo_epi16(v00, _mm_set1_epi16((short)( 1.28033 * 128)));
            __m128i dG = _mm_add_epi16(_mm_mullo_epi16(u00, _mm_set1_epi16((short)(-0.21482 * 128))), _mm_mullo_epi16(v00, _mm_set1_epi16((short)(-0.38059 * 128))));
            __m128i dB =               _mm_mullo_epi16(u00, _mm_set1_epi16((short)( 2.12798 * 128)));

            __m128i xR[2] = { _mm_unpacklo_epi16(dR, dR), _mm_unpackhi_epi16(dR, dR) };
            __m128i xG[2] = { _mm_unpacklo_epi16(dG, dG), _mm_unpackhi_epi16(dG, dG) };
            __m128i xB[2] = { _mm_unpacklo_epi16(dB, dB), _mm_unpackhi_epi16(dB, dB) };

            __m128i t[4];
            __m128i b[4];

            t[0] = _mm_packus_epi16(_mm_srai_epi16(_mm_add_epi16(y00, xR[0]), 7), _mm_srai_epi16(_mm_add_epi16(y01, xR[1]), 7));
            t[1] = _mm_packus_epi16(_mm_srai_epi16(_mm_add_epi16(y00, xG[0]), 7), _mm_srai_epi16(_mm_add_epi16(y01, xG[1]), 7));
            t[2] = _mm_packus_epi16(_mm_srai_epi16(_mm_add_epi16(y00, xB[0]), 7), _mm_srai_epi16(_mm_add_epi16(y01, xB[1]), 7));
            t[3] = _mm_set1_epi8(-1);
            b[0] = _mm_packus_epi16(_mm_srai_epi16(_mm_add_epi16(y10, xR[0]), 7), _mm_srai_epi16(_mm_add_epi16(y11, xR[1]), 7));
            b[1] = _mm_packus_epi16(_mm_srai_epi16(_mm_add_epi16(y10, xG[0]), 7), _mm_srai_epi16(_mm_add_epi16(y11, xG[1]), 7));
            b[2] = _mm_packus_epi16(_mm_srai_epi16(_mm_add_epi16(y10, xB[0]), 7), _mm_srai_epi16(_mm_add_epi16(y11, xB[1]), 7));
            b[3] = _mm_set1_epi8(-1);

            _MM_TRANSPOSE4_EPI8(t[0], t[1], t[2], t[3]);
            _MM_TRANSPOSE4_EPI8(b[0], b[1], b[2], b[3]);

            _mm_storeu_si128((__m128i*)rgb0, t[0]); rgb0 += 16;
            _mm_storeu_si128((__m128i*)rgb0, t[1]); rgb0 += 16;
            _mm_storeu_si128((__m128i*)rgb0, t[2]); rgb0 += 16;
            _mm_storeu_si128((__m128i*)rgb0, t[3]); rgb0 += 16;
            _mm_storeu_si128((__m128i*)rgb1, b[0]); rgb1 += 16;
            _mm_storeu_si128((__m128i*)rgb1, b[1]); rgb1 += 16;
            _mm_storeu_si128((__m128i*)rgb1, b[2]); rgb1 += 16;
            _mm_storeu_si128((__m128i*)rgb1, b[3]); rgb1 += 16;
        }
        continue;
#endif
        for (int w = 0; w < halfWidth; ++w)
        {
            int y00 = (*y0++) * 128;
            int y01 = (*y0++) * 128;
            int y10 = (*y1++) * 128;
            int y11 = (*y1++) * 128;
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
            int dR =                               v00 * (int)( 1.28033 * 128);
            int dG = u00 * (int)(-0.21482 * 128) + v00 * (int)(-0.38059 * 128);
            int dB = u00 * (int)( 2.12798 * 128);

            auto clamp = [](int value) -> unsigned char
            {
                return (unsigned char)(value < 255 ? value < 0 ? 0 : value : 255);
            };

            (*rgb0++) = clamp((y00 + dR) >> 7);
            (*rgb0++) = clamp((y00 + dG) >> 7);
            (*rgb0++) = clamp((y00 + dB) >> 7);
            (*rgb0++) = 255;
            (*rgb0++) = clamp((y01 + dR) >> 7);
            (*rgb0++) = clamp((y01 + dG) >> 7);
            (*rgb0++) = clamp((y01 + dB) >> 7);
            (*rgb0++) = 255;
            (*rgb1++) = clamp((y10 + dR) >> 7);
            (*rgb1++) = clamp((y10 + dG) >> 7);
            (*rgb1++) = clamp((y10 + dB) >> 7);
            (*rgb1++) = 255;
            (*rgb1++) = clamp((y11 + dR) >> 7);
            (*rgb1++) = clamp((y11 + dG) >> 7);
            (*rgb1++) = clamp((y11 + dB) >> 7);
            (*rgb1++) = 255;
        }
    }
}
//------------------------------------------------------------------------------
void yuv2rgb_yu12(int width, int height, const void* yuv, void* rgb, int rgbWidth, int strideRGB, int alignWidth, int alignHeight)
{
    int sizeY = align(width, alignWidth) * align(height, alignHeight);
    int sizeUV = align(width / 2, alignWidth) * align(height / 2, alignHeight);

    if (strideRGB == 0)
        strideRGB = rgbWidth * width;

    if (rgbWidth == 3)
        yuv2rgb<3, false, false>(width, height, yuv, (char*)yuv + sizeY, (char*)yuv + sizeY + sizeUV, width, width / 2, width / 2, rgb, strideRGB);
    else if (rgbWidth == 4)
        yuv2rgb<4, false, false>(width, height, yuv, (char*)yuv + sizeY, (char*)yuv + sizeY + sizeUV, width, width / 2, width / 2, rgb, strideRGB);
}
//------------------------------------------------------------------------------
void yuv2rgb_yv12(int width, int height, const void* yuv, void* rgb, int rgbWidth, int strideRGB, int alignWidth, int alignHeight)
{
    int sizeY = align(width, alignWidth) * align(height, alignHeight);
    int sizeUV = align(width / 2, alignWidth) * align(height / 2, alignHeight);

    if (strideRGB == 0)
        strideRGB = rgbWidth * width;

    if (rgbWidth == 3)
        yuv2rgb<3, false, true>(width, height, yuv, (char*)yuv + sizeY + sizeUV, (char*)yuv + sizeY, width, width / 2, width / 2, rgb, strideRGB);
    else if (rgbWidth == 4)
        yuv2rgb<4, false, true>(width, height, yuv, (char*)yuv + sizeY + sizeUV, (char*)yuv + sizeY, width, width / 2, width / 2, rgb, strideRGB);
}
//------------------------------------------------------------------------------
void yuv2rgb_nv12(int width, int height, const void* yuv, void* rgb, int rgbWidth, int strideRGB, int alignWidth, int alignHeight)
{
    int sizeY = align(width, alignWidth) * align(height, alignHeight);

    if (strideRGB == 0)
        strideRGB = rgbWidth * width;

    if (rgbWidth == 3)
        yuv2rgb<3, true, false>(width, height, yuv, (char*)yuv + sizeY, (char*)yuv + sizeY + 1, width, width, width, rgb, strideRGB);
    else if (rgbWidth == 4)
        yuv2rgb<4, true, false>(width, height, yuv, (char*)yuv + sizeY, (char*)yuv + sizeY + 1, width, width, width, rgb, strideRGB);
}
//------------------------------------------------------------------------------
void yuv2rgb_nv21(int width, int height, const void* yuv, void* rgb, int rgbWidth, int strideRGB, int alignWidth, int alignHeight)
{
    int sizeY = align(width, alignWidth) * align(height, alignHeight);

    if (strideRGB == 0)
        strideRGB = rgbWidth * width;

    if (rgbWidth == 3)
        yuv2rgb<3, true, true>(width, height, yuv, (char*)yuv + sizeY + 1, (char*)yuv + sizeY, width, width, width, rgb, strideRGB);
    else if (rgbWidth == 4)
        yuv2rgb<4, true, true>(width, height, yuv, (char*)yuv + sizeY + 1, (char*)yuv + sizeY, width, width, width, rgb, strideRGB);
}
//------------------------------------------------------------------------------
