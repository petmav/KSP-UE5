# drive: full wheel throttle on flat ground, 40 s after a 2 s settle; reference = the C# motor model without tyre slip (same tick, drive ramp, torque curve, traction control, motors enabled per the craft) accelerating the vessel mass plus the wheels' spin inertia (sum I / r^2, VPP floors I at 0.01)

| craft | native | friction | warp | mass / with wheel inertia (t) | motors | t to 5 m/s sim / ref (s) | t to 10 m/s | v at 10 s | v at 20 s | v at 30 s | v at 40 s | v max sim / ref | ref asymptote | max fwd slip (m/s) | tick (ms) |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| bug-e | explicit | lineal | 1x | 1.287 / 4.533 | 6 | 10.86 / 9.46 | 38.82 / -1.00 | 4.67 / 5.22 | 7.74 / 8.01 | 9.31 / 9.33 | 10.07 / 9.99 | 10.07 / 9.99 | 12.00 | 1.00 | 0.0071 |
| prospector | explicit | lineal | 1x | 1.249 / 1.428 | 2 | 8.98 / 3.28 | 20.96 / 7.04 | 5.43 / 13.34 | 9.60 / 21.56 | 13.80 / 26.75 | 17.81 / 30.35 | 17.81 / 30.35 | 42.50 | 12.32 | 0.0074 |
