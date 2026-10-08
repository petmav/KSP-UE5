using UnityEngine;

namespace VehiclePhysics
{
	public class Clutch : Block
	{
		public float lockRatio = 1f;

		public float inertiaRatio = 0.1f;

		private float L;

		private float I;

		private float T;

		private Connection m_input;

		private Connection m_output;

		protected override void Initialize()
		{
			SetInputs(1);
			SetOutputs(1);
		}

		public override bool CheckConnections()
		{
			m_input = base.inputs[0];
			m_output = base.outputs[0];
			if (m_input != null)
			{
				return m_output != null;
			}
			return false;
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
			I = m_output.I * inertiaRatio;
			m_input.L = Mathf.Lerp(L, m_output.L, lockRatio);
			m_input.I = Mathf.Lerp(I, m_output.I, lockRatio);
			m_input.Tr = m_output.Tr * lockRatio;
		}

		public override void EvaluateTorqueDownstream()
		{
			m_output.outTd = m_input.outTd * lockRatio;
			float viscousLockingDt = Solver.GetViscousLockingDt(lockRatio);
			float b = (I * m_input.L / m_input.I - L) / viscousLockingDt;
			T = Mathf.Lerp(m_input.outTd, b, lockRatio);
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
