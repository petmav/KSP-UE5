# C7 analysis: .NET Framework 4.8 x64 vs KSP Mono

- .NET: .NET Framework CLR 4.0.30319.42000 x64 (RyuJIT)
- Mono: Mono 5.11.0 (Visual Studio built mono) (Environment.Version 4.0.30319.42000)
- inputs: libm 3093 values, f32 10000 rows; System.Random regenerated in Mono: random7 mismatches 0, random11 mismatches 0

### f64: 25 functions, 77325 values

| fn | n | differ | signed-zero | NaN/Inf | finite | max ULP (finite) | more accurate | first examples |
|---|---|---|---|---|---|---|---|---|
| Math.Acos | 3093 | 0 | 0 | 0 | 0 | 0 |  |  |
| Math.Asin | 3093 | 0 | 0 | 0 | 0 | 0 |  |  |
| Math.Atan | 3093 | 0 | 0 | 0 | 0 | 0 |  |  |
| Math.Atan2(0.7,x) | 3093 | 0 | 0 | 0 | 0 | 0 |  |  |
| Math.Atan2(x,0.7) | 3093 | 1 | 1 | 0 | 0 | 0 |  | x=-0.0: .NET -0.0, Mono 0.0 [signed zero] |
| Math.Ceiling | 3093 | 147 | 147 | 0 | 0 | 0 |  | x=-0.5: .NET 0.0, Mono -0.0 [signed zero]<br>x=-1e-09: .NET 0.0, Mono -0.0 [signed zero]<br>x=-1.000001000001e-09: .NET 0.0, Mono -0.0 [signed zero] |
| Math.Cos | 3093 | 0 | 0 | 0 | 0 | 0 |  |  |
| Math.Cosh | 3093 | 0 | 0 | 0 | 0 | 0 |  |  |
| Math.Exp | 3093 | 0 | 0 | 0 | 0 | 0 |  |  |
| Math.Floor | 3093 | 0 | 0 | 0 | 0 | 0 |  |  |
| Math.IEEERemainder(x,2pi) | 3093 | 0 | 0 | 0 | 0 | 0 |  |  |
| Math.Log | 3093 | 0 | 0 | 0 | 0 | 0 |  |  |
| Math.Log10 | 3093 | 0 | 0 | 0 | 0 | 0 |  |  |
| Math.Pow(1.0000001,x) | 3093 | 0 | 0 | 0 | 0 | 0 |  |  |
| Math.Pow(x,0.4) | 3093 | 0 | 0 | 0 | 0 | 0 |  |  |
| Math.Pow(x,2.0) | 3093 | 0 | 0 | 0 | 0 | 0 |  |  |
| Math.Pow(|x|,1/3) | 3093 | 0 | 0 | 0 | 0 | 0 |  |  |
| Math.Round | 3093 | 0 | 0 | 0 | 0 | 0 |  |  |
| Math.Round(AwayFromZero) | 3093 | 0 | 0 | 0 | 0 | 0 |  |  |
| Math.Sin | 3093 | 0 | 0 | 0 | 0 | 0 |  |  |
| Math.Sinh | 3093 | 0 | 0 | 0 | 0 | 0 |  |  |
| Math.Sqrt | 3093 | 0 | 0 | 0 | 0 | 0 |  |  |
| Math.Tan | 3093 | 244 | 0 | 4 | 240 | 269687594237 | mono better 122, dotnet better 118 | x=3.141592653589793: .NET -1.2246063538223773e-16, Mono -1.2246467991473532e-16 [164065729543 ulp (err .NET 1.64e+11 ulp, Mono 0.121 ulp)]<br>x=1.5707963267948966: .NET 1.6331778728383844e+16, Mono 1.633123935319537e+16 [269687594237 ulp (err .NET 2.7e+11 ulp, Mono 0.122 ulp)]<br>x=6.283185307179586: .NET -2.4492127076447545e-16, Mono -2.4492935982947064e-16 [164065729543 ulp (err .NET 1.64e+11 ulp, Mono 0.121 ulp)] |
| Math.Tanh | 3093 | 0 | 0 | 0 | 0 | 0 |  |  |
| x%2pi | 3093 | 0 | 0 | 0 | 0 | 0 |  |  |

### f32: 15 functions, 46395 values

| fn | n | differ | signed-zero | NaN/Inf | finite | max ULP (finite) | more accurate | first examples |
|---|---|---|---|---|---|---|---|---|
| (float)Math.Acos(f) | 3093 | 0 | 0 | 0 | 0 | 0 |  |  |
| (float)Math.Asin(f) | 3093 | 0 | 0 | 0 | 0 | 0 |  |  |
| (float)Math.Atan(f) | 3093 | 0 | 0 | 0 | 0 | 0 |  |  |
| (float)Math.Atan2(f,0.7f) | 3093 | 1 | 1 | 0 | 0 | 0 |  | x=-0.0: .NET -0.0, Mono 0.0 [signed zero] |
| (float)Math.Cos(f) | 3093 | 0 | 0 | 0 | 0 | 0 |  |  |
| (float)Math.Exp(f) | 3093 | 0 | 0 | 0 | 0 | 0 |  |  |
| (float)Math.Floor(f) | 3093 | 0 | 0 | 0 | 0 | 0 |  |  |
| (float)Math.Log(f) | 3093 | 0 | 0 | 0 | 0 | 0 |  |  |
| (float)Math.Log10(f) | 3093 | 0 | 0 | 0 | 0 | 0 |  |  |
| (float)Math.Pow(f,0.4f) | 3093 | 0 | 0 | 0 | 0 | 0 |  |  |
| (float)Math.Round(f) | 3093 | 0 | 0 | 0 | 0 | 0 |  |  |
| (float)Math.Sin(f) | 3093 | 0 | 0 | 0 | 0 | 0 |  |  |
| (float)Math.Sqrt(f) | 3093 | 0 | 0 | 0 | 0 | 0 |  |  |
| (float)Math.Tan(f) | 3093 | 5 | 0 | 2 | 3 | 287 |  | x=999999986991104.0: .NET 9.438694953918457, Mono 9.438578605651855 [122 ulp]<br>x=1.0000000272564224e+16: .NET -0.5720774531364441, Mono -0.5720945596694946 [287 ulp]<br>x=9.999999778196308e+21: .NET nan, Mono -1.081023931503296 [special] |
| f%360f | 3093 | 0 | 0 | 0 | 0 | 0 |  |  |

### f32ops (rows matching each model; A = per-op float32, B/B' = double intermediates, see models())

| probe | n | .NET != Mono | max ULP | .NET matches | Mono matches |
|---|---|---|---|---|---|
| poly | 10000 | 5096 | 164 | {'A+B': 4904, 'A': 5096} | {'A+B': 4904, 'B': 5096} |
| dot2 | 10000 | 3382 | 1510 | {'A+B': 6618, 'A': 3382} | {'A+B': 6618, 'B': 3382} |
| cast_each | 10000 | 0 | 0 | {'A+B': 10000} | {'A+B': 10000} |
| acc_loop | 10000 | 7094 | 7 | {'A': 5967, 'A+B': 1309, "A+B'": 1127, "A+B+B'": 1597} | {'B': 3578, "B+B'": 3516, 'A+B': 1309, "A+B+B'": 1597} |
| dot2_call | 10000 | 3382 | 1510 | {'A+B': 6618, 'A': 3382} | {'A+B': 6618, 'B': 3382} |
| locals | 10000 | 2024 | 8192 | {"A+B+B'+B''": 6051, 'A': 1637, "A+B''": 1006, "A+B+B''": 701, "A+B+B'": 165, "A+B'+B''": 218, "A+B'": 184, 'A+B': 38} | {"A+B+B'+B''": 6051, "B+B'+B''": 1025, "A+B''": 1006, "A+B+B''": 701, "B''": 545, "A+B'+B''": 218, "B+B''": 387, "B'+B''": 67} |
| sqrt_div | 10000 | 1474 | 2 | {'A+B': 8526, 'A': 1474} | {'A+B': 8526, 'B': 1474} |
| compare | 10000 | 10000 | 0 | {"A+B'": 10000} | {'B': 10000} |
| arg_noinline | 10000 | 0 | 0 | {'A+B': 7400, 'A': 2600} | {'A+B': 7400, 'A': 2600} |
| arg_inline | 10000 | 0 | 0 | {'A+B': 7400, 'A': 2600} | {'A+B': 7400, 'A': 2600} |

### f32ops_wide (rows matching each model; A = per-op float32, B/B' = double intermediates, see models())

| probe | n | .NET != Mono | max ULP | .NET matches | Mono matches |
|---|---|---|---|---|---|
| ret_noinline | 10000 | 0 | 0 | {'A': 10000} | {'A': 10000} |
| ret_inline | 10000 | 0 | 0 | {'A': 10000} | {'A': 10000} |
| field | 10000 | 0 | 0 | {'A': 10000} | {'A': 10000} |
| struct_field | 10000 | 0 | 0 | {'A': 10000} | {'A': 10000} |
| static_field | 10000 | 0 | 0 | {'A': 10000} | {'A': 10000} |
| local | 10000 | 0 | 0 | {'A': 10000} | {'A': 10000} |
| mul_wide | 10000 | 10000 | 268410176 | {'A': 10000} | {'B': 10000} |
| muladd_wide | 10000 | 10000 | 46939209728 | {'A': 10000} | {'B': 10000} |

### conv: 7 differences

| key | column | input | .NET | Mono |
|---|---|---|---|---|
| ('(ulong)d', 0) | r | nan | 0 | -9223372036854775808 |
| ('(ulong)d', 1) | r | inf | 0 | -9223372036854775808 |
| ('(ulong)d', 32) | r | 1.8446744073709552e+19 | 0 | -9223372036854775808 |
| ('(ulong)d', 34) | r | 1e+20 | 0 | -9223372036854775808 |
| ('(ulong)d', 36) | r | 1e+30 | 0 | -9223372036854775808 |
| ('(ulong)d', 37) | r | 3.4028234663852886e+38 | 0 | -9223372036854775808 |
| ('(ulong)d', 38) | r | 1e+300 | 0 | -9223372036854775808 |

### iconv: 1 differences

| key | column | input | .NET | Mono |
|---|---|---|---|---|
| ('(double)ulong', 6) | r | '9223372036854776833' | 9.223372036854776e+18 | 9.223372036854778e+18 |

### tostring: 1 differences

| key | column | input | .NET | Mono |
|---|---|---|---|---|
| ('d.ToString()', 287) | s | 5.997450259893145 | '5.99745025989314' | '5.99745025989315' |

### parse: 0 differences

### sysrand: 0 differences
