# xxYUV
Convert between RGB and YUV

## Performance

|        | i9-7980XE | YU12 | YV12 | NV12 | NV21 |
| ------ | ----------| ---- | ---- | ---- | ---- |
| xxYUV  | SSE2      | 200  | 200  | 163  | 163  |
| xxYUV  | AVX2      | 125  | 125  | 110  | 110  |
| xxYUV  | AXV512    | 114  | 114  | 116  | 116  |
| ------ | ----------| ---- | ---- | ---- | ---- |
| libyuv | SSE2      | 240  | 240  | 223  | 223  |
| libyuv | AVX2      | 160  | 160  | 151  | 151  |
