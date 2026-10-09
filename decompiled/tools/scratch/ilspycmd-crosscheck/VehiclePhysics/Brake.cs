using UnityEngine;

namespace VehiclePhysics
{
	public class Brake : Block
	{
		public float brakeInput;

		public float maxBrakeTorque = 500f;

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
			if (m_output != null)
			{
				return m_output.input.GetType() == typeof(Wheel);
			}
			return false;
		}

		public override void ComputeStateUpstream()
		{
			if (m_input != null)
			{
				m_input.L = m_output.L;
				m_input.I = m_output.I;
				m_input.Tr = m_output.Tr;
			}
		}

		public override void EvaluateTorqueDownstream()
		{
			if (m_input != null)
			{
				m_output.outTd = m_input.outTd;
			}
			float brakeTorque = Mathf.Clamp01(brakeInput) * maxBrakeTorque;
			((Wheel)m_output.input).AddBrakeTorque(brakeTorque);
		}
	}
}
