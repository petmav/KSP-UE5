# brake: accelerate to v0, then wheel throttle 0 and the Brakes action group (brakeTweakable from the craft); flat ground. Reference = KSP's brake torque (brakeResponse ramp; maxBrakeTorqueRest = 1000 kN m once brakeState >= 1, i.e. brakeTweakable >= 100 %) through rolling wheels, capped by locked wheels sliding on the tyre curve (no load transfer); 'locked' = wheels locked from the first tick

| craft | native | friction | v0 (m/s) | stop distance sim (m) | stop time sim (s) | ref: distance / time | ref locked: distance | max decel above 1 m/s (m/s2) | max tilt (deg) |
|---|---|---|---|---|---|---|---|---|---|
| prospector | explicit | smooth | 8.00 | 7.34 | 1.22 | 6.05 / 1.22 | 1.23 | 23.96 | 11.2 |
| prospector | jolt-vc | smooth | 8.01 | 7.39 | 1.28 | 6.05 / 1.22 | 1.23 | 22.15 | 17.2 |
