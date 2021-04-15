# xxYUV
Convert between RGB and YUV

## Benchmark Environment
https://github.com/metarutaiga/xxImGui/tree/experimental

## Performance (macOS) Encode / Decode
                                                    
|            |           | Encode      |             | Decode      |             |
| ---------- | --------- | ----------- | ----------- | ----------- | ----------- |
|            | Apple M1  | YU12 / YV12 | NV12 / NV21 | YU12 / YV12 | NV12 / NV21 |
| xxYUV      | NEON      |          37 |          38 |          38 |          42 |
| xxYUV      | AMX       |           ? |           ? |          67 |           ? |
| xxYUV      | SSE2      |         134 |         133 |          58 |          56 |
| Accelerate | NEON      |          33 |          35 |          62 |          59 |
| Accelerate | SSE2      |         139 |         138 |         232 |         231 |
| libyuv     | NEON      |          48 |          49 |         122 |          89 |
| libyuv     | SSE2      |         146 |         146 |         171 |         164 |

|            |           | Encode      |             | Decode      |             |
| ---------- | --------- | ----------- | ----------- | ----------- | ----------- |
|            | i7-8700B  | YU12 / YV12 | NV12 / NV21 | YU12 / YV12 | NV12 / NV21 |
| xxYUV      | SSE2      |          50 |          51 |          69 |          62 |
| xxYUV      | AVX2      |          31 |          33 |          46 |          39 |
| Accelerate | AVX2      |          31 |          33 |          67 |          62 |
| libyuv     | SSE2      |          60 |          61 |          87 |          82 |
| libyuv     | AVX2      |          48 |          39 |          60 |          54 |
