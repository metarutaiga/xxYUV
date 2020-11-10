//==============================================================================
// xxYUV : yuv Source
//
// Copyright (c) 2020 TAiGA
// https://github.com/metarutaiga/xxYUV
//==============================================================================
#if defined(_M_ARM64) || defined(_M_HYBRID_X86_ARM64)
#   include <arm64_neon.h>
#elif defined(__ARM_NEON__) || defined(__ARM_NEON) || defined(_M_ARM) || defined(_M_ARM64)
#   include <arm_neon.h>
#elif defined(_M_IX86) || defined(_M_AMD64) || defined(__i386__) || defined(__amd64__)
#   include <immintrin.h>
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
#if defined(_M_IX86) || defined(_M_AMD64) || defined(__i386__) || defined(__amd64__)
        int halfWidth8 = halfWidth / 8;
        for (int w = 0; w < halfWidth8; ++w)
        {
            __m128i y00 = _mm_loadu_si128((__m128i*)y0); y0 += 16;
            __m128i y10 = _mm_loadu_si128((__m128i*)y1); y1 += 16;

            __m128i u00;
            __m128i v00;
            if (interleaved)
            {
                if (firstU)
                {
                    __m128i uv00 = _mm_loadu_si128((__m128i*)u0); u0 += 16;
                    u00 = _mm_and_si128(uv00, _mm_set1_epi16(0xFF));
                    v00 = _mm_srli_epi16(uv00, 8);
                }
                else
                {
                    __m128i uv00 = _mm_loadu_si128((__m128i*)v0); v0 += 16;
                    u00 = _mm_srli_epi16(uv00, 8);
                    v00 = _mm_and_si128(uv00, _mm_set1_epi16(0xFF));
                }
                u00 = _mm_packus_epi16(u00, u00);
                v00 = _mm_packus_epi16(v00, v00);
            }
            else
            {
                u00 = _mm_loadl_epi64((__m128i*)u0); u0 += 8;
                v00 = _mm_loadl_epi64((__m128i*)v0); v0 += 8;
            }
            u00 = _mm_unpacklo_epi8(u00, u00);
            v00 = _mm_unpacklo_epi8(v00, v00);

            __m128i t[4];
            __m128i b[4];

            t[iY] = y00;
            t[iU] = u00;
            t[iV] = v00;
            t[iA] = _mm_set1_epi8(-1);
            b[iY] = y10;
            b[iU] = u00;
            b[iV] = v00;
            b[iA] = _mm_set1_epi8(-1);

            _MM_TRANSPOSE4_EPI8(t[0], t[1], t[2], t[3]);
            _MM_TRANSPOSE4_EPI8(b[0], b[1], b[2], b[3]);

            _mm_storeu_si128((__m128i*)output0 + 0, t[0]);
            _mm_storeu_si128((__m128i*)output0 + 1, t[1]);
            _mm_storeu_si128((__m128i*)output0 + 2, t[2]);
            _mm_storeu_si128((__m128i*)output0 + 3, t[3]); output0 += 16 * 4;
            _mm_storeu_si128((__m128i*)output1 + 0, b[0]);
            _mm_storeu_si128((__m128i*)output1 + 1, b[1]);
            _mm_storeu_si128((__m128i*)output1 + 2, b[2]);
            _mm_storeu_si128((__m128i*)output1 + 3, b[3]); output1 += 16 * 4;
        }
        continue;
#endif
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
