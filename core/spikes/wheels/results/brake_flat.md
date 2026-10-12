# brake: accelerate to v0, then wheel throttle 0 and the Brakes action group (brakeTweakable from the craft); flat ground. Reference = KSP's brake torque (brakeResponse ramp; maxBrakeTorqueRest = 1000 kN m once brakeState >= 1, i.e. brakeTweakable >= 100 %) through rolling wheels, capped by locked wheels sliding on the tyre curve (no load transfer); 'locked' = wheels locked from the first tick

| craft | native | friction | v0 (m/s) | stop distance sim (m) | stop time sim (s) | ref: distance / time | ref locked: distance | max decel above 1 m/s (m/s2) | max tilt (deg) |
|---|---|---|---|---|---|---|---|---|---|
| bug-e | explicit | smooth | 8.00 | 8.30 | 1.30 | 8.04 / 1.38 | 1.23 | 29.48 | 12.2 |
| bug-e | jolt-vc | smooth | 8.00 | 8.20 | 1.32 | 8.04 / 1.38 | 1.23 | 26.83 | 15.6 |
| prospector | explicit | smooth | 20.02 | 23.57 | 2.10 | 21.66 / 1.68 | 7.70 | 71.02 | 53.3 |
| prospector | jolt-vc | smooth | 20.02 | 31.39 | 6.64 | 21.66 / 1.68 | 7.71 | 63.13 | 178.9 |
