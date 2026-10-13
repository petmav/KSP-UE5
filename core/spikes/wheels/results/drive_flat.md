# drive: full wheel throttle on flat ground, 40 s after a 2 s settle; reference = the C# motor model without tyre slip (same tick, drive ramp, torque curve, traction control, motors enabled per the craft) accelerating the vessel mass plus the wheels' spin inertia (sum I / r^2, VPP floors I at 0.01)

| craft | native | friction | warp | mass / with wheel inertia (t) | motors | t to 5 m/s sim / ref (s) | t to 10 m/s | v at 10 s | v at 20 s | v at 30 s | v at 40 s | v max sim / ref | ref asymptote | max fwd slip (m/s) | tick (ms) |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| bug-e | explicit | smooth | 1x | 1.287 / 4.533 | 6 | 5.02 / 5.68 | 21.30 / 23.54 | 7.95 / 7.36 | 9.89 / 9.70 | 10.71 / 10.55 | 10.71 / 10.71 | 10.71 / 10.71 | 12.00 | 0.05 | 0.0079 |
| bug-e | explicit | smooth | 4x | 1.287 / 4.533 | 6 | 5.92 / 5.68 | 26.64 / 23.44 | 6.86 / 7.39 | 8.44 / 9.71 | 9.77 / 10.56 | 9.22 / 10.72 | 10.94 / 10.72 | 12.00 | 10.72 | 0.0083 |
| bug-e | jolt-vc | smooth | 1x | 1.287 / 4.533 | 6 | 4.60 / 5.68 | 21.54 / 23.54 | 8.03 / 7.36 | 9.87 / 9.70 | 10.71 / 10.55 | 10.71 / 10.71 | 10.72 / 10.71 | 12.00 | 0.01 | 0.0097 |
| bug-e | jolt-vc | smooth | 4x | 1.287 / 4.533 | 6 | 4.24 / 5.68 | 14.80 / 23.44 | 8.77 / 7.39 | 10.34 / 9.71 | 11.04 / 10.56 | 10.74 / 10.72 | 11.05 / 10.72 | 12.00 | 0.25 | 0.0097 |
| prospector | explicit | smooth | 1x | 1.249 / 1.428 | 2 | 2.14 / 2.08 | 4.30 / 4.26 | 19.41 / 19.41 | 28.36 / 28.35 | 33.36 / 33.40 | 37.01 / 37.15 | 37.01 / 37.15 | 42.50 | 0.00 | 0.0055 |
| prospector | explicit | smooth | 4x | 1.249 / 1.428 | 2 | 3.92 / 2.16 | 6.72 / 4.56 | 15.01 / 17.77 | 23.90 / 25.40 | 27.09 / 29.64 | 29.73 / 32.49 | 30.69 / 32.49 | 42.50 | 30.81 | 0.0059 |
| prospector | jolt-vc | smooth | 1x | 1.249 / 1.428 | 2 | 2.14 / 2.08 | 4.30 / 4.26 | 19.42 / 19.41 | 28.36 / 28.35 | 33.36 / 33.40 | 37.00 / 37.15 | 37.00 / 37.15 | 42.50 | 0.00 | 0.0061 |
| prospector | jolt-vc | smooth | 4x | 1.249 / 1.428 | 2 | 2.16 / 2.16 | 4.48 / 4.56 | 17.84 / 17.77 | 25.42 / 25.40 | 29.59 / 29.64 | 32.36 / 32.49 | 32.36 / 32.49 | 42.50 | 0.00 | 0.0063 |
