using UnityEngine;

namespace VehiclePhysics
{
	public class DifferentialGeneric : Block
	{
		public float torqueGeometry = 0.5f;

		public float stiffness = 0.2f;

		public float damping = 1f;

		private Connection m_input;

		private Connection m_output1;

		private Connection m_output2;

		protected override void Initialize()
		{
			SetInputs(1);
			SetOutputs(2);
		}

		public override bool CheckConnections()
		{
			m_input = base.inputs[0];
			m_output1 = base.outputs[0];
			m_output2 = base.outputs[1];
			if (m_input != null && m_output1 != null)
			{
				return m_output2 != null;
			}
			return false;
		}

		public override void ComputeStateUpstream()
		{
			float num = Mathf.Clamp01(2f * (1f - torqueGeometry));
			float num2 = Mathf.Clamp01(2f * torqueGeometry);
			float num3 = m_output1.I * num + m_output2.I * num2;
			float num4 = num3 / (num + num2);
			float num5 = 1f / m_output1.I;
			float num6 = 1f / m_output2.I;
			m_input.L = num4 * (m_output1.L * num5 * num + m_output2.L * num6 * num2);
			m_input.I = num3;
			float num7 = num * (1f - stiffness) + stiffness;
			float num8 = num2 * (1f - stiffness) + stiffness;
			m_input.Tr = num4 * (m_output1.Tr * num5 * num7 + m_output2.Tr * num6 * num8);
		}

		public override void EvaluateTorqueDownstream()
		{
			m_output1.outTd = m_input.outTd * (1f - torqueGeometry);
			m_output2.outTd = m_input.outTd * torqueGeometry;
			float deltaTime = Solver.deltaTime;
			float num = 1f / ((m_output1.I + m_output2.I) * deltaTime) * (m_output1.I * (m_output2.L + (m_output2.outTd + m_output2.Tr * damping) * deltaTime) - m_output2.I * (m_output1.L + (m_output1.outTd + m_output1.Tr * damping) * deltaTime));
			num *= stiffness;
			m_output1.outTd += num;
			m_output2.outTd -= num;
		}
	}
}
