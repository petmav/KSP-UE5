# C3 native maths: convention check vs double-precision textbook formulas

Error per row = max |native - reference| / (2^-23 * max |reference component|), i.e. float32 epsilons relative to the vector/quaternion/matrix magnitude. Reference = textbook formula in double on the float32 inputs. Inverse reference = conjugate (that is what Unity returns; it does not divide by |q|^2).

| fn | rows | max error (eps rel. to magnitude) | skipped rows | worst example (inputs -> ref / native) |
|---|---|---|---|---|
| Quaternion.Euler | 56 | 1.58 | 0 | [720.0, 360.0, -240.0] -> [-0.0, 0.0, 0.8660254, 0.5] / [2e-07, -1e-07, 0.8660254, 0.5000001] |
| Quaternion.AngleAxis | 84 | 1.47 | 0 | [720.0, 1.0, 0.0, 0.0] -> [-0.0, -0.0, -0.0, 1.0] / [2e-07, 0.0, 0.0, 1.0] |
| Quaternion.Inverse | 60 | 0.00 | 0 |  |
| Matrix4x4.TRS | 45 | 0.73 | 0 | [0.0, 0.0, 0.0, 0.391904, 0.200562, 0.360423, 0.822363, 2.0, 0.5, 3.0] -> [1.3194792, 1.5, -0.0947344, 0.0, -0.2177978, 0.2165064, 0.3945745, 0.0, 1.8371174, -1.4999998, 1.8371173, 0.0, 0.0, 0.0, 0.0, 1.0] / [1.3194792,  |
| Matrix4x4.TRS.inverse | 45 | 1.88 | 0 | [100000.0, 20000.0, -300000.0, 0.391904, 0.200562, 0.360423, 0.822363, 2.0, 0.5, 3.0] -> [0.3298698, -0.8711915, 0.2041242, 0.0, 0.375, 0.8660255, -0.1666667, 0.0, -0.0236836, 1.5782983, 0.2041242, 0.0, -47592.0697429, 5 |
| Transform.TransformPoint | 200 | 2.38 | 10 | [0.0, 0.0, 0.0, 0.707107, 0.0, 0.0, 0.707107, 2.0, 0.5, 3.0, -0.3, 0.9, -0.2] -> [-0.6, 0.6, 0.45] / [-0.6, 0.6, 0.4500002] |
| Transform.TransformDirection | 180 | 1.49 | 30 | [0.0, 0.0, 0.0, 0.707107, 0.0, 0.0, 0.707107, 1.0, 1.0, 1.0, -0.3, 0.9, -0.2] -> [-0.3, 0.2, 0.9] / [-0.3, 0.1999999, 0.9000002] |
| Transform.TransformVector | 180 | 2.38 | 30 | [0.0, 0.0, 0.0, 0.707107, 0.0, 0.0, 0.707107, 2.0, 0.5, 3.0, -0.3, 0.9, -0.2] -> [-0.6, 0.6, 0.45] / [-0.6, 0.6, 0.4500002] |
| Transform.InverseTransformDirection | 180 | 1.58 | 30 | [0.0, 0.0, 0.0, 0.707107, 0.0, 0.0, 0.707107, 1.0, 1.0, 1.0, -0.3, 0.9, -0.2] -> [-0.3, -0.2, -0.9] / [-0.3, -0.2000002, -0.9] |
| Transform.InverseTransformPoint | 200 | 6.92 | 10 | [0.0, 0.0, 0.0, 0.707107, 0.0, 0.0, 0.707107, 2.0, 0.5, 3.0, -0.3, 0.9, -0.2] -> [-0.15, -0.4, -0.3] / [-0.15, -0.4000003, -0.3] |
| Quaternion.Lerp | 245 | 1.01 | 0 | [0.0, 0.707107, 0.0, -0.707107, 0.5, 0.5, -0.5, 0.5, 1.0] -> [0.5, 0.5, -0.5, 0.5] / [0.5, 0.5, -0.5, 0.5000001] |
| Quaternion.LerpUnclamped | 245 | 1.02 | 0 | [0.1, -0.2, 0.3, 0.9, 0.5, 0.5, -0.5, 0.5, 1.5] -> [0.4816146, 0.5848178, -0.6192188, 0.2064063] / [0.4816147, 0.5848178, -0.6192189, 0.2064063] |

- Quaternion.eulerAngles round trip: max |Euler(q.eulerAngles) - q| = 0.000542 over 56 quaternions (sign-aligned).
