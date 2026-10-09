# C2 vector functions: fitted formulas (float32 emulation, sin/cos/sqrt/pow rounded from double)

| fn | candidate | components | bit-exact | max ULP |
|---|---|---|---|---|
| insideUnitCircle | theta=Range(0,2pi) v1, r=sqrt(Range01 v2) (r cos, r sin) | 96 | 95 | 1 |
| insideUnitCircle | theta=2pi v1, r=sqrt(v2) (r sin, r cos) | 96 | 0 | 2113215534 |
| insideUnitCircle | theta=Range(0,2pi) v1, r=sqrt(Range01 v2) (r sin, r cos) | 96 | 0 | 2119216356 |
| insideUnitCircle | theta=2pi v1, r=sqrt(v2) (r cos, r sin) | 96 | 0 | 2120818858 |
| insideUnitCircle | theta=2pi v2, r=sqrt(v1) (r sin, r cos) | 96 | 0 | 2122147728 |
| insideUnitCircle | theta=2pi v2, r=sqrt(v1) (r cos, r sin) | 96 | 0 | 2125614955 |
| insideUnitCircle | theta=Range(0,2pi) v2, r=sqrt(Range01 v1) (r cos, r sin) | 96 | 0 | 2125623666 |
| insideUnitCircle | theta=Range(0,2pi) v2, r=sqrt(Range01 v1) (r sin, r cos) | 96 | 0 | 2127823235 |
| insideUnitSphere | unit A * cbrt_value(v3) | 144 | 144 | 0 |
| insideUnitSphere | unit D * cbrt_value(v3) | 144 | 48 | 2127353982 |
| insideUnitSphere | unit B * cbrt_value(v3) | 144 | 12 | 2128322284 |
| insideUnitSphere | unit A * cbrt_range01(v3) | 144 | 0 | 19640211 |
| insideUnitSphere | unit A * linear_range01(v3) | 144 | 0 | 42850449 |
| insideUnitSphere | unit C * linear_range01(v3) | 144 | 0 | 2117401998 |
| insideUnitSphere | unit B * linear_range01(v3) | 144 | 0 | 2119382992 |
| insideUnitSphere | unit D * linear_range01(v3) | 144 | 0 | 2119382993 |
| insideUnitSphere | unit C * cbrt_range01(v3) | 144 | 0 | 2119644765 |
| insideUnitSphere | unit D * cbrt_range01(v3) | 144 | 0 | 2122720143 |
| insideUnitSphere | unit B * cbrt_range01(v3) | 144 | 0 | 2123359569 |
| insideUnitSphere | unit C * cbrt_value(v3) | 144 | 0 | 2125884251 |
| onUnitSphere | A z=Range(-1,1) v1, a=Range(0,2pi) v2 | 144 | 144 | 0 |
| onUnitSphere | D z=Range(-1,1), a=Range(-pi,pi) | 144 | 48 | 2130564911 |
| onUnitSphere | B z=2v1-1, a=2pi v2 | 144 | 8 | 2130685272 |
| onUnitSphere | C a from v1, z from v2 | 144 | 0 | 2126254313 |
| rotation | Normalize(Range(-1,1) x4), /n, then w>=0 | 192 | 192 | 0 |
| rotation | Normalize(Range(-1,1) x4), *1/n, then w>=0 | 192 | 150 | 1 |
| rotation | Normalize(2v-1 x4), /n, then w>=0 | 192 | 150 | 2 |
| rotation | Normalize(2v-1 x4), *1/n, then w>=0 | 192 | 122 | 2 |
| rotation | Normalize(Range(-1,1) x4), /n | 192 | 108 | 2126734300 |
| rotation | Normalize(Range(-1,1) x4), *1/n | 192 | 83 | 2126734299 |
| rotation | Normalize(2v-1 x4), /n | 192 | 70 | 2129151394 |
| rotation | Normalize(2v-1 x4), *1/n | 192 | 56 | 2129151394 |
| rotationUniform | u1=Range01,u2=Range(0,2pi),u3=Range(0,2pi) (a sin u2, a cos u2, b sin u3, b cos u3), then w>=0 | 192 | 192 | 0 |
| rotationUniform | u1=Range01,u2=Range(0,2pi),u3=Range(0,2pi) (a sin u2, a cos u2, b sin u3, b cos u3) | 192 | 92 | 2130312052 |
| rotationUniform | u1=Range01,u2=Range(0,2pi),u3=Range(0,2pi) (b sin u3, b cos u3, a sin u2, a cos u2), then w>=0 | 192 | 0 | 2119766917 |
| rotationUniform | u1=Range01,u2=Range(0,2pi),u3=Range(0,2pi) (a sin u2, a cos u2, b sin u3, b cos u3) xyzw->wxyz | 192 | 0 | 2120449930 |
| rotationUniform | u1=v1,u2=2pi v2,u3=2pi v3 (a sin u2, a cos u2, b sin u3, b cos u3) | 192 | 0 | 2120592445 |
| rotationUniform | u1=Range01,u2=Range(0,2pi),u3=Range(0,2pi) (a sin u2, a cos u2, b sin u3, b cos u3) xyzw->wxyz, then w>=0 | 192 | 0 | 2120657255 |
| rotationUniform | u1=Range01,u2=Range(0,2pi),u3=Range(0,2pi) (b sin u3, b cos u3, a sin u2, a cos u2) | 192 | 0 | 2120804938 |
| rotationUniform | u1=v1,u2=2pi v2,u3=2pi v3 (a sin u2, a cos u2, b sin u3, b cos u3), then w>=0 | 192 | 0 | 2120806826 |
| rotationUniform | u1=v1,u2=2pi v2,u3=2pi v3 (a sin u2, a cos u2, b sin u3, b cos u3) xyzw->wxyz, then w>=0 | 192 | 0 | 2125441531 |
| rotationUniform | u1=v1,u2=2pi v2,u3=2pi v3 (a sin u2, a cos u2, b sin u3, b cos u3) xyzw->wxyz | 192 | 0 | 2125711958 |
| rotationUniform | u1=v1,u2=2pi v2,u3=2pi v3 (b sin u3, b cos u3, a sin u2, a cos u2) | 192 | 0 | 2126314249 |
| rotationUniform | u1=v1,u2=2pi v2,u3=2pi v3 (b sin u3, b cos u3, a sin u2, a cos u2), then w>=0 | 192 | 0 | 2128867749 |

Best per function:
- insideUnitCircle: theta=Range(0,2pi) v1, r=sqrt(Range01 v2) (r cos, r sin): 95/96 bit-exact, max 1 ULP
- insideUnitSphere: unit A * cbrt_value(v3): 144/144 bit-exact, max 0 ULP
- onUnitSphere: A z=Range(-1,1) v1, a=Range(0,2pi) v2: 144/144 bit-exact, max 0 ULP
- rotation: Normalize(Range(-1,1) x4), /n, then w>=0: 192/192 bit-exact, max 0 ULP
- rotationUniform: u1=Range01,u2=Range(0,2pi),u3=Range(0,2pi) (a sin u2, a cos u2, b sin u3, b cos u3), then w>=0: 192/192 bit-exact, max 0 ULP
