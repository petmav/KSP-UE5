using UnityEngine;

namespace VehiclePhysics
{
	public class SynchronousDrive : Block
	{
		public float targetRpm;

		public float maxTorque = 100f;

		public float damping = 0.95f;

		private Connection m_output;

		public float angularVelocity => m_output.L / m_output.I;

		public float torque => m_output.outTd;

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

		public override void EvaluateTorqueDownstream()
		{
			float value = (m_output.I * targetRpm * Block.RpmToW - m_output.L) / Solver.deltaTime - m_output.Tr * damping;
			m_output.outTd = Mathf.Clamp(value, 0f - maxTorque, maxTorque);
		}
	}
}
