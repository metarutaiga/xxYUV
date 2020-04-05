//==============================================================================
// xxYUV : yuv2rgb Source
//
// Copyright (c) 2020 TAiGA
// https://github.com/metarutaiga/xxYUV
//==============================================================================
#if defined(__ARM_NEON__) || defined(__ARM_NEON)
#include <arm_neon.h>
#endif
#include "yuv2rgb.h"

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
#if defined(__ARM_NEON__) || defined(__ARM_NEON)
        for (int w = 0; w < halfWidth; w += 8)
        {
            uint8x16_t y00lh = vld1q_u8(y0); y0 += 16;
            uint8x16_t y10lh = vld1q_u8(y1); y1 += 16;
            int16x8_t y00 = vreinterpretq_s16_u16(vshll_n_u8(vget_low_u8(y00lh), 7));
            int16x8_t y01 = vreinterpretq_s16_u16(vshll_n_u8(vget_high_u8(y00lh), 7));
            int16x8_t y10 = vreinterpretq_s16_u16(vshll_n_u8(vget_low_u8(y10lh), 7));
            int16x8_t y11 = vreinterpretq_s16_u16(vshll_n_u8(vget_high_u8(y10lh), 7));

            uint8x8x2_t uv00lh;
            if (interleaved)
            {
                if (firstV)
                {
                    uint8x16_t uv00 = vld1q_u8(v0); v0 += 16;
                    uint8x8x2_t uv00lhx = vuzp_u8(vget_low_u8(uv00), vget_high_u8(uv00));
                    uv00lh.val[0] = uv00lhx.val[1];
                    uv00lh.val[1] = uv00lhx.val[0];
                }
                else
                {
                    uint8x16_t uv00 = vld1q_u8(u0); u0 += 16;
                    uv00lh = vuzp_u8(vget_low_u8(uv00), vget_high_u8(uv00));
                }
            }
            else
            {
                uv00lh.val[0] = vld1_u8(u0); u0 += 8;
                uv00lh.val[1] = vld1_u8(v0); v0 += 8;
            }
            int16x8_t u00 = vreinterpretq_s16_u16(vsubl_u8(uv00lh.val[0], vdup_n_u8(128)));
            int16x8_t v00 = vreinterpretq_s16_u16(vsubl_u8(uv00lh.val[1], vdup_n_u8(128)));

            int16x8_t dR =                                                        vmulq_n_s16(v00, (int16_t)( 1.28033 * 128));
            int16x8_t dG = vaddq_s16(vmulq_n_s16(u00, (int16_t)(-0.21482 * 128)), vmulq_n_s16(v00, (int16_t)(-0.38059 * 128)));
            int16x8_t dB =           vmulq_n_s16(u00, (int16_t)( 2.12798 * 128));

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
void yuv2rgb_yu12(int width, int height, const void* yuv, void* rgb, int rgbWidth, int strideRGB)
{
    int sizeY = width * height;
    int sizeUV = width / 2 * height / 2;

    if (strideRGB == 0)
        strideRGB = rgbWidth * width;

    if (rgbWidth == 3)
        yuv2rgb<3, false, false>(width, height, yuv, (char*)yuv + sizeY, (char*)yuv + sizeY + sizeUV, width, width / 2, width / 2, rgb, rgbWidth * width);
    else if (rgbWidth == 4)
        yuv2rgb<4, false, false>(width, height, yuv, (char*)yuv + sizeY, (char*)yuv + sizeY + sizeUV, width, width / 2, width / 2, rgb, rgbWidth * width);
}
//------------------------------------------------------------------------------
void yuv2rgb_yv12(int width, int height, const void* yuv, void* rgb, int rgbWidth, int strideRGB)
{
    int sizeY = width * height;
    int sizeUV = width / 2 * height / 2;

    if (strideRGB == 0)
        strideRGB = rgbWidth * width;

    if (rgbWidth == 3)
        yuv2rgb<3, false, true>(width, height, yuv, (char*)yuv + sizeY + sizeUV, (char*)yuv + sizeY, width, width / 2, width / 2, rgb, rgbWidth * width);
    else if (rgbWidth == 4)
        yuv2rgb<4, false, true>(width, height, yuv, (char*)yuv + sizeY + sizeUV, (char*)yuv + sizeY, width, width / 2, width / 2, rgb, rgbWidth * width);
}
//------------------------------------------------------------------------------
void yuv2rgb_nv12(int width, int height, const void* yuv, void* rgb, int rgbWidth, int strideRGB)
{
    int sizeY = width * height;

    if (strideRGB == 0)
        strideRGB = rgbWidth * width;

    if (rgbWidth == 3)
        yuv2rgb<3, true, false>(width, height, yuv, (char*)yuv + sizeY, (char*)yuv + sizeY + 1, width, width, width, rgb, rgbWidth * width);
    else if (rgbWidth == 4)
        yuv2rgb<4, true, false>(width, height, yuv, (char*)yuv + sizeY, (char*)yuv + sizeY + 1, width, width, width, rgb, rgbWidth * width);
}
//------------------------------------------------------------------------------
void yuv2rgb_nv21(int width, int height, const void* yuv, void* rgb, int rgbWidth, int strideRGB)
{
    int sizeY = width * height;

    if (strideRGB == 0)
        strideRGB = rgbWidth * width;

    if (rgbWidth == 3)
        yuv2rgb<3, true, true>(width, height, yuv, (char*)yuv + sizeY + 1, (char*)yuv + sizeY, width, width, width, rgb, rgbWidth * width);
    else if (rgbWidth == 4)
        yuv2rgb<4, true, true>(width, height, yuv, (char*)yuv + sizeY + 1, (char*)yuv + sizeY, width, width, width, rgb, rgbWidth * width);
}
//------------------------------------------------------------------------------
