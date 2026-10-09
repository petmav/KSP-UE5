namespace VehiclePhysics
{
	public class Gearset : Block
	{
		public class Settings
		{
			public float[] ratios = new float[2] { 1f, 2f };
		}

		public int gearInput;

		public Settings settings = new Settings();

		private Connection m_input;

		private Connection m_output;

		private float m_ratio = 1f;

		private float m_invRatio = 1f;

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

		public override void PreStep()
		{
			int num = settings.ratios.Length;
			if (num == 0)
			{
				gearInput = 0;
				m_ratio = 1f;
			}
			else
			{
				if (gearInput < 0)
				{
					gearInput = 0;
				}
				else if (gearInput >= num)
				{
					gearInput = num - 1;
				}
				m_ratio = settings.ratios[gearInput];
			}
			m_invRatio = 1f / m_ratio;
		}

		public override void ComputeStateUpstream()
		{
			m_input.L = m_output.L * m_invRatio;
			m_input.I = m_output.I * m_invRatio * m_invRatio;
			m_input.Tr = m_output.Tr * m_invRatio;
		}

		public override void EvaluateTorqueDownstream()
		{
			m_output.outTd = m_input.outTd * m_ratio;
		}
	}
}
