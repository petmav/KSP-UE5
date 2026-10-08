using System.Collections.Generic;
using UnityEngine;

namespace VehiclePhysics
{
	public class Solver
	{
		private class StateVector
		{
			private Block.State[] m_states;

			public Block.State[] states => m_states;

			public StateVector(int length)
			{
				m_states = new Block.State[length];
			}
		}

		private class DerivativeVector
		{
			private Block.Derivative[] m_derivatives;

			public Block.Derivative[] derivatives => m_derivatives;

			public DerivativeVector(int length)
			{
				m_derivatives = new Block.Derivative[length];
			}
		}

		private Block[] m_blocks = new Block[0];

		private int m_wheelCount;

		private StateVector[] m_states;

		private DerivativeVector[] m_derivatives;

		private bool m_RK4enabled;

		public const float minInertia = 0.01f;

		public const float minWheelRadius = 0.01f;

		public const float minWheelMass = 0.01f;

		public static float viscousCouplingRate = 100f;

		public static float viscousLockedRatio = 0.95f;

		public static float time { get; private set; }

		public static float deltaTime { get; private set; }

		public string resultMessage { get; private set; }

		public static void SetDebugTime(float t, float dt)
		{
		}

		public bool Initialize(Wheel[] wheels, bool enableRK4)
		{
			List<Block> list = new List<Block>();
			bool flag = true;
			resultMessage = "Initializing | ";
			Wheel[] array = wheels;
			foreach (Wheel item in array)
			{
				list.Add(item);
			}
			m_wheelCount = wheels.Length;
			int num = 0;
			while (num < list.Count)
			{
				int count = list.Count;
				for (int j = num; j < count; j++)
				{
					Block.Connection[] inputs = list[j].inputs;
					foreach (Block.Connection connection in inputs)
					{
						if (connection != null && !list.Contains(connection.output))
						{
							Block output = connection.output;
							list.Add(output);
							bool flag2 = output.CheckConnections();
							flag = flag && flag2;
							if (!flag2)
							{
								resultMessage += $"Failed: {output}@{j} | ";
							}
						}
					}
				}
				num = count;
			}
			if (!flag)
			{
				list = new List<Block>();
				array = wheels;
				foreach (Wheel item2 in array)
				{
					list.Add(item2);
				}
			}
			m_blocks = list.ToArray();
			if (enableRK4)
			{
				m_states = new StateVector[2];
				m_derivatives = new DerivativeVector[5];
			}
			else
			{
				m_states = new StateVector[1];
				m_derivatives = new DerivativeVector[1];
			}
			m_RK4enabled = enableRK4;
			int k = 0;
			for (int num2 = m_states.Length; k < num2; k++)
			{
				m_states[k] = new StateVector(m_blocks.Length);
			}
			int l = 0;
			for (int num3 = m_derivatives.Length; l < num3; l++)
			{
				m_derivatives[l] = new DerivativeVector(m_blocks.Length);
			}
			resultMessage += (flag ? "Ok." : "Aborted.");
			return flag;
		}

		public void Integrate(float t, float dt, int steps, bool useRK4)
		{
			if (m_RK4enabled && useRK4)
			{
				IntegrateRK4(m_states[0].states, m_states[1].states, m_derivatives[0].derivatives, m_derivatives[1].derivatives, m_derivatives[2].derivatives, m_derivatives[3].derivatives, m_derivatives[4].derivatives, t, dt);
			}
			else
			{
				IntegrateEuler(m_states[0].states, m_derivatives[0].derivatives, t, dt, steps);
			}
		}

		public static float GetViscousLockingDt(float lockRatio)
		{
			float num = Mathf.Max(deltaTime, 1f / viscousCouplingRate);
			if (lockRatio > viscousLockedRatio)
			{
				num = ((!(lockRatio >= 1f)) ? Mathf.Lerp(num, deltaTime, Mathf.InverseLerp(viscousLockedRatio, 1f, lockRatio)) : deltaTime);
			}
			return num;
		}

		private static void EulerStep(ref Block.State In, ref Block.Derivative Der, ref Block.State Out)
		{
			Out.L = In.L + Der.T * deltaTime;
			Out.P = In.P + Der.F * deltaTime;
			Out.Lr = In.Lr + Der.Tr * deltaTime;
		}

		private static void EulerStepL(ref Block.State In, ref Block.Derivative Der, ref Block.State Out)
		{
			Out.L = In.L + Der.T * deltaTime;
			Out.Lr = In.Lr + Der.Tr * deltaTime;
		}

		private void EulerStep(Block.State[] In, Block.Derivative[] Der, Block.State[] Out)
		{
			int i = 0;
			for (int wheelCount = m_wheelCount; i < wheelCount; i++)
			{
				EulerStep(ref In[i], ref Der[i], ref Out[i]);
			}
			int j = m_wheelCount;
			for (int num = In.Length; j < num; j++)
			{
				EulerStepL(ref In[j], ref Der[j], ref Out[j]);
			}
		}

		private void IntegrateEuler(Block.State[] S, Block.Derivative[] D, float t, float dt, int subSteps)
		{
			time = t;
			deltaTime = dt;
			int i = 0;
			for (int num = m_blocks.Length; i < num; i++)
			{
				m_blocks[i].PreStep();
				m_blocks[i].GetState(ref S[i]);
			}
			float num2 = dt / (float)subSteps;
			for (int j = 0; j < subSteps; j++)
			{
				time = t;
				deltaTime = num2;
				ComputeDerivative(S, D);
				EulerStep(S, D, S);
				t += num2;
			}
			time = t + dt;
			deltaTime = dt;
			int k = 0;
			for (int num3 = m_blocks.Length; k < num3; k++)
			{
				m_blocks[k].SetState(S[k]);
			}
		}

		private void EvaluateRK4(Block.State[] S, Block.State[] Stemp, Block.Derivative[] DIn, float t, float dt, Block.Derivative[] DOut)
		{
			time = t;
			deltaTime = dt;
			EulerStep(S, DIn, Stemp);
			time = t + dt;
			ComputeDerivative(Stemp, DOut);
		}

		private void ComputeRK4Derivative(ref Block.Derivative A, ref Block.Derivative B, ref Block.Derivative C, ref Block.Derivative D, ref Block.Derivative DOut)
		{
			DOut.T = 1f / 6f * (A.T + 2f * (B.T + C.T) + D.T);
			DOut.F = 1f / 6f * (A.F + 2f * (B.F + C.F) + D.F);
			DOut.Tr = 1f / 6f * (A.Tr + 2f * (B.Tr + C.Tr) + D.Tr);
		}

		private void ComputeRK4DerivativeT(ref Block.Derivative A, ref Block.Derivative B, ref Block.Derivative C, ref Block.Derivative D, ref Block.Derivative DOut)
		{
			DOut.T = 1f / 6f * (A.T + 2f * (B.T + C.T) + D.T);
			DOut.Tr = 1f / 6f * (A.Tr + 2f * (B.Tr + C.Tr) + D.Tr);
		}

		private void ComputeRK4Derivative(Block.Derivative[] A, Block.Derivative[] B, Block.Derivative[] C, Block.Derivative[] D, Block.Derivative[] DOut)
		{
			int i = 0;
			for (int wheelCount = m_wheelCount; i < wheelCount; i++)
			{
				ComputeRK4Derivative(ref A[i], ref B[i], ref C[i], ref D[i], ref DOut[i]);
			}
			int j = m_wheelCount;
			for (int num = A.Length; j < num; j++)
			{
				ComputeRK4DerivativeT(ref A[j], ref B[j], ref C[j], ref D[j], ref DOut[j]);
			}
		}

		private void IntegrateRK4(Block.State[] S, Block.State[] Stemp, Block.Derivative[] D, Block.Derivative[] Da, Block.Derivative[] Db, Block.Derivative[] Dc, Block.Derivative[] Dd, float t, float dt)
		{
			time = t;
			deltaTime = dt;
			int i = 0;
			for (int num = m_blocks.Length; i < num; i++)
			{
				m_blocks[i].PreStep();
				m_blocks[i].GetState(ref S[i]);
			}
			ComputeDerivative(S, Da);
			EvaluateRK4(S, Stemp, Da, t, dt * 0.5f, Db);
			EvaluateRK4(S, Stemp, Db, t, dt * 0.5f, Dc);
			EvaluateRK4(S, Stemp, Dc, t, dt, Dd);
			time = t;
			deltaTime = dt;
			ComputeRK4Derivative(Da, Db, Dc, Dd, D);
			EulerStep(S, D, S);
			time = t + dt;
			int j = 0;
			for (int num2 = m_blocks.Length; j < num2; j++)
			{
				m_blocks[j].SetState(S[j]);
			}
		}

		private void ComputeDerivative(Block.State[] S, Block.Derivative[] Out)
		{
			int i = 0;
			for (int num = m_blocks.Length; i < num; i++)
			{
				m_blocks[i].SetSubstepState(S[i]);
				m_blocks[i].ComputeStateUpstream();
			}
			for (int num2 = m_blocks.Length - 1; num2 >= 0; num2--)
			{
				m_blocks[num2].EvaluateTorqueDownstream();
				m_blocks[num2].GetSubstepDerivative(ref Out[num2]);
			}
		}

		public Block[] GetBlockArray()
		{
			return m_blocks;
		}
	}
}
