namespace VehiclePhysics
{
	public class TorqueInjector : Block
	{
		public float torque;

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
			m_input.L = m_output.L;
			m_input.I = m_output.I;
			m_input.Tr = m_output.Tr;
		}

		public override void EvaluateTorqueDownstream()
		{
			m_output.outTd = m_input.outTd + torque;
		}
	}
}
