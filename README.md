# xxYUV
Convert between RGB and YUV

## Performance (macOS)

|            | Apple M1  | YU12 | YV12 | NV12 | NV21 |
| ---------- | ----------| ---- | ---- | ---- | ---- |
| xxYUV      | NEON      | 38   | 39   | 42   | 42   |
| xxYUV      | AMX       | 67   | 67   | ?    | ?    |
| Accelerate | NEON      | 62   | 62   | 59   | 59   |
| libyuv     | NEON      | 122  | 122  | 89   | 88   |

|            | i7-8700B  | YU12 | YV12 | NV12 | NV21 |
| ---------- | ----------| ---- | ---- | ---- | ---- |
| xxYUV      | SSE2      | 69   | 69   | 62   | 62   |
| xxYUV      | AVX2      | 46   | 46   | 39   | 39   |
| Accelerate | AVX2      | 67   | 67   | 62   | 62   |
| libyuv     | SSE2      | 87   | 87   | 82   | 85   |
| libyuv     | AVX2      | 60   | 60   | 54   | 56   |

## Performance (Windows)

|        | i9-7980XE | YU12 | YV12 | NV12 | NV21 |
| ------ | ----------| ---- | ---- | ---- | ---- |
| xxYUV  | SSE2      | 200  | 200  | 163  | 163  |
| xxYUV  | AVX2      | 125  | 125  | 110  | 110  |
| xxYUV  | AXV512    | 114  | 114  | 116  | 116  |
| libyuv | SSE2      | 240  | 240  | 223  | 223  |
| libyuv | AVX2      | 160  | 160  | 151  | 151  |
