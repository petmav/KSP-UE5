# drive: full wheel throttle on pqs ground, 30 s after a 2 s settle; reference = the C# motor model without tyre slip (same tick, drive ramp, torque curve, traction control, motors enabled per the craft) accelerating the vessel mass plus the wheels' spin inertia (sum I / r^2, VPP floors I at 0.01)

| craft | native | friction | warp | mass / with wheel inertia (t) | motors | t to 5 m/s sim / ref (s) | t to 10 m/s | v at 10 s | v at 20 s | v at 30 s | v at 40 s | v max sim / ref | ref asymptote | max fwd slip (m/s) | tick (ms) |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| bug-e | explicit | smooth | 1x | 1.287 / 4.533 | 6 | 5.00 / 5.68 | 21.26 / 23.54 | 7.95 / 7.36 | 9.89 / 9.70 | 10.72 / 10.55 | 0.00 / 0.00 | 10.72 / 10.55 | 12.00 | 0.05 | 0.0114 |
| bug-e | jolt-vc | smooth | 1x | 1.287 / 4.533 | 6 | 4.60 / 5.68 | 21.48 / 23.54 | 8.03 / 7.36 | 9.88 / 9.70 | 10.72 / 10.55 | 0.00 / 0.00 | 10.72 / 10.55 | 12.00 | 0.01 | 0.0134 |
| prospector | explicit | smooth | 1x | 1.249 / 1.428 | 2 | 2.14 / 2.08 | 4.30 / 4.26 | 19.42 / 19.41 | 28.39 / 28.35 | 33.44 / 33.40 | 0.00 / 0.00 | 33.44 / 33.40 | 42.50 | 0.00 | 0.0082 |
| prospector | jolt-vc | smooth | 1x | 1.249 / 1.428 | 2 | 2.14 / 2.08 | 4.30 / 4.26 | 19.42 / 19.41 | 28.39 / 28.35 | 33.43 / 33.40 | 0.00 / 0.00 | 33.43 / 33.40 | 42.50 | 0.00 | 0.0091 |
