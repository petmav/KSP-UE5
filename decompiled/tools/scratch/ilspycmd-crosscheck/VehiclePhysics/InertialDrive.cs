using UnityEngine;

namespace VehiclePhysics
{
	public class InertialDrive : Block
	{
		public float motorInput;

		public float maxMotorTorque = 100f;

		public float clutchInput;

		public float inertia = 0.5f;

		public float u1 = 10f;

		public float u2;

		public float u3 = 0.03f;

		public float maxRpm = 500f;

		public float damping = 0.95f;

		private Connection m_output;

		private float L;

		private float I = 1f;

		private float T;

		public float rpm => L / I * Block.WToRpm;

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

		private float ComputeInertia()
		{
			if (!(inertia < 0.01f))
			{
				return inertia;
			}
			return 0.01f;
		}

		public override void GetState(ref State S)
		{
			I = ComputeInertia();
			S.L = L;
		}

		public override void SetSubstepState(State S)
		{
			L = S.L;
		}

		public override void EvaluateTorqueDownstream()
		{
			float num = motorInput * maxMotorTorque;
			float num2 = Mathf.Abs(L);
			float num3 = I * Mathf.Abs(maxRpm) * Block.RpmToW;
			if (num2 > num3)
			{
				num = 0f;
			}
			float num4 = num2 / I;
			float num5 = (I * u1 + num4 * (u2 + u3 * u3 * num4)) * Mathf.Sign(0f - L);
			float num6 = L / I;
			float num7 = num + num5;
			float num8 = (m_output.L + L + (m_output.Tr * damping + num7) * Solver.deltaTime) / (m_output.I + I);
			float num9 = I * (num8 - num6) / Solver.deltaTime - num7;
			num9 *= Mathf.Clamp01(1f - clutchInput);
			m_output.outTd = 0f - num9;
			T = num9 + num7;
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
