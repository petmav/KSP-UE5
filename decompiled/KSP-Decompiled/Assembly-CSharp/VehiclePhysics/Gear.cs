namespace VehiclePhysics
{
	public class Gear : Block
	{
		public float ratio = 1f;

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

		public override void ComputeStateUpstream()
		{
			float num = 1f / ratio;
			m_input.L = m_output.L * num;
			m_input.I = m_output.I * num * num;
			m_input.Tr = m_output.Tr * num;
		}

		public override void EvaluateTorqueDownstream()
		{
			m_output.outTd = m_input.outTd * ratio;
		}
	}
}
