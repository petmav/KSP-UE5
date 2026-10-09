# C2 analysis: UnityEngine.Random model vs capture

- InitState: 29/29 seeds match `s0=seed; s(i+1)=s(i)*1812433253+1`
- xorshift128 step: 60/60 traced states reproduced by the step function
- Random.value mapping hits on 60 traced calls: {'(r & 0x7FFFFF) / 8388607f': 60, '(r & 0x7FFFFF) * (1f/8388607f)': 60}
- value table: 1856/1856 values match `(r & 0x7FFFFF) / 8388607f`; state after 64 values: 29/29 seeds
- Range(-10f,10f): 1856 calls; formula hits {'v*min + (1-v)*max': 1856}; one draw per call: state after 64 calls 29/29
- Range(0,7) + Range(-1000,1000): 3712 calls; formula hits {'min + r % (max-min)': 3712, 'min + r % (uint)(max-min) (wrap)': 3712}; state after 128 calls 29/29
- Range(int.MinValue,int.MaxValue) traced: 60 calls; hits {'min + r % (max-min)': 60, 'min + r % (uint)(max-min) (wrap)': 60}
- edge calls replayed up to the first rotationUniform: 406/406 match; mismatches: []

Draw counts: after_vec = 16 x insideUnitSphere + 16 x onUnitSphere + 16 x rotation + 16 x insideUnitCircle.
- seed 0: after_vec reached after 176 draws
- seed 0: after_edge reached after 35 draws
- seed 1: after_vec reached after 176 draws
- seed 1: after_edge reached after 35 draws
- seed 2: after_vec reached after 176 draws
- seed 2: after_edge reached after 35 draws
- DiscoveryInfo.GetClassRadius: 45/45 radii reproduced by base * Range(0.75f,1.25f) with the model

ksp_asteroid rows: seed 0: Ast. MMC-921, class C, 33.7137 days, ref Sun, sma 15804043038.432005, err None; seed 1: Ast. UEG-570, class B, 15.0142 days, ref Sun, sma 14652454321.459547, err None; seed 42: Ast. MZB-394, class C, 59.9614 days, ref Sun, sma 14219039313.407183, err None; seed 12345: Ast. GFC-589, class C, 33.5917 days, ref Sun, sma 13750632404.889017, err None; seed 24301: Ast. FOO-886, class D, 21.7358 days, ref Sun, sma 14071139114.4924, err None
