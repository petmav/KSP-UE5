# brake: accelerate to v0, then wheel throttle 0 and the Brakes action group (brakeTweakable from the craft); flat ground. Reference = KSP's brake torque (brakeResponse ramp; maxBrakeTorqueRest = 1000 kN m once brakeState >= 1, i.e. brakeTweakable >= 100 %) through rolling wheels, capped by locked wheels sliding on the tyre curve (no load transfer); 'locked' = wheels locked from the first tick

| craft | native | friction | v0 (m/s) | stop distance sim (m) | stop time sim (s) | ref: distance / time | ref locked: distance | max decel above 1 m/s (m/s2) | max tilt (deg) |
|---|---|---|---|---|---|---|---|---|---|
| bug-e | explicit | lineal | 8.00 | 8.30 | 1.30 | 8.03 / 1.36 | 1.22 | 28.82 | 12.2 |
| bug-e | jolt-vc | lineal | 8.00 | 8.20 | 1.32 | 8.03 / 1.36 | 1.22 | 26.81 | 15.6 |
| prospector | explicit | lineal | 20.02 | 23.56 | 2.10 | 21.64 / 1.66 | 7.69 | 71.14 | 53.3 |
| prospector | jolt-vc | lineal | 20.02 | 31.37 | 6.56 | 21.65 / 1.66 | 7.69 | 63.60 | 179.5 |
