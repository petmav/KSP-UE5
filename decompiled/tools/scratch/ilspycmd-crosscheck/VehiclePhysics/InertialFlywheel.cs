using UnityEngine;

namespace VehiclePhysics
{
	public class InertialFlywheel : Block
	{
		public float torque;

		public float lockRatio = 1f;

		public float inertia = 1f;

		public float maxRpm = 10f;

		public float damping = 1f;

		public float viscousCouplingRate = 100f;

		public float viscousLockedRatio = 0.95f;

		private float L;

		private float T;

		private float m_viscousDt;

		private Connection m_output;

		public float Tlock { get; private set; }

		public float rpm => L / inertia * Block.WToRpm;

		protected override void Initialize()
		{
			SetInputs(0);
			SetOutputs(1);
		}

		public override bool CheckConnections()
		{
			m_output = base.outputs[0];
			return m_output != null;
		}

		public override void PreStep()
		{
			lockRatio = Mathf.Clamp01(lockRatio);
			inertia = Mathf.Max(inertia, 0.01f);
			m_viscousDt = 1f / viscousCouplingRate;
		}

		public override void GetState(ref State S)
		{
			S.L = L;
		}

		public override void SetSubstepState(State S)
		{
			L = S.L;
		}

		public override void ComputeStateUpstream()
		{
		}

		public override void EvaluateTorqueDownstream()
		{
			float num = inertia * Mathf.Abs(maxRpm) * Block.RpmToW;
			float max = (num - L) / Solver.deltaTime - m_output.Tr * damping;
			float min = (0f - num - L) / Solver.deltaTime - m_output.Tr * damping;
			float num2 = Mathf.Clamp(torque, min, max);
			float a = Mathf.Max(Solver.deltaTime, m_viscousDt);
			a = Mathf.Lerp(a, Solver.deltaTime, Mathf.InverseLerp(viscousLockedRatio, 1f, lockRatio));
			Tlock = 1f / ((inertia + m_output.I) * a) * (m_output.I * (L + num2 * a) - inertia * (m_output.L + m_output.Tr * damping * a));
			T = num2 - Tlock * lockRatio;
			m_output.outTd = Tlock * lockRatio;
		}

		public override void GetSubstepDerivative(ref Derivative D)
		{
			D.T = T;
		}

		public override void SetState(State S)
		{
			L = S.L;
		}

		public void Reset()
		{
			L = 0f;
		}
	}
}
