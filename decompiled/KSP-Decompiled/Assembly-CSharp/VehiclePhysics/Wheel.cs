using EdyCommonTools;
using UnityEngine;

namespace VehiclePhysics
{
	public class Wheel : Block
	{
		public TireFriction tireFriction;

		public float radius;

		public float mass;

		private float I;

		private float invI;

		public float load;

		public float grip;

		public Vector2 V;

		public Vector2 Fext;

		private float m_brakeTorque;

		public float L;

		public Vector2 P;

		private float w;

		public float T;

		public Vector2 F;

		public float Tr;

		public bool isResting;

		public float m_forwardSlip;

		public Vector2 m_tireForce;

		public float m_Ty;

		public bool m_isAdherent;

		public Vector2 m_adherentSlip;

		public Vector2 m_adherentForce;

		private float m_inTd;

		private float m_inTb;

		private TireFriction.ContactPatch m_contact = new TireFriction.ContactPatch();

		public float debug1;

		public float debug2;

		public float debug3;

		public float inertia => I;

		public float angularVelocity => w;

		public float driveTorque => m_inTd;

		public float brakeTorque => m_inTb;

		public TireFriction.ContactPatch contactPatch => m_contact;

		public void RecalculateConstants()
		{
			I = mass * radius * radius * 0.5f;
			if (I < 0.01f)
			{
				I = 0.01f;
			}
			invI = 1f / I;
		}

		public float AddBrakeTorque(float brakeTorque)
		{
			m_brakeTorque += brakeTorque;
			if (m_brakeTorque < 0f)
			{
				m_brakeTorque = 0f;
			}
			return m_brakeTorque;
		}

		public void ResetBrakeTorque()
		{
			m_brakeTorque = 0f;
		}

		public void RecalculateVars()
		{
			w = L * invI;
		}

		protected override void Initialize()
		{
			SetInputs(1);
			SetOutputs(0);
			tireFriction = new TireFriction();
		}

		public override void GetState(ref State S)
		{
			S.L = L;
			S.P = Vector2.zero;
			S.Lr = 0f;
			isResting = true;
		}

		public override void SetSubstepState(State S)
		{
			L = S.L;
			P = S.P;
			RecalculateVars();
		}

		public override void ComputeStateUpstream()
		{
			m_forwardSlip = (float)((double)w * (double)radius - (double)V.y);
			Vector2 slip = new Vector2(V.x, m_forwardSlip);
			m_contact.load = load;
			m_contact.slip = slip;
			m_contact.groundGrip = grip;
			m_tireForce = tireFriction.GetForce(m_contact);
			m_Ty = m_tireForce.y * radius;
			m_isAdherent = tireFriction.IsAdherentSlip(m_contact);
			if (m_isAdherent)
			{
				m_adherentSlip.x = slip.x;
				m_adherentSlip.y = tireFriction.GetAdherentSlipForward(m_contact);
				m_contact.slip = m_adherentSlip;
				m_contact.slip.y *= Mathf.Sign(slip.y);
				m_adherentForce = tireFriction.GetAdherentForce(m_contact);
				float num = m_adherentForce.y * radius;
				if (MathUtility.FastAbs(m_Ty) < MathUtility.FastAbs(num))
				{
					m_Ty = num;
				}
			}
			else
			{
				m_adherentSlip = Vector2.zero;
				m_adherentForce = Vector2.zero;
			}
			Connection connection = base.inputs[0];
			if (connection != null)
			{
				connection.L = L;
				connection.I = I;
				connection.Tr = Tr;
			}
		}

		public override void EvaluateTorqueDownstream()
		{
			m_inTd = base.inputs[0]?.outTd ?? 0f;
			m_inTb = m_brakeTorque;
			float num = m_inTb * Mathf.Sign(0f - L);
			float tsum = m_inTd + m_Ty + num;
			float num2 = (0f - L) / Solver.deltaTime;
			float num3 = (float)((double)V.y / (double)radius - (double)w) * I / Solver.deltaTime;
			float num4 = MathUtility.FastAbs(num2);
			float num5 = MathUtility.FastAbs(num3);
			if (!(num4 < num5) && MathUtility.FastAbs(num5 - num4) >= 1E-06f)
			{
				tsum = SolveSymmetricConstraint(tsum, m_Ty, num3);
				tsum = SolveSymmetricConstraint(tsum, num, num2);
			}
			else
			{
				tsum = SolveSymmetricConstraint(tsum, num, num2);
				tsum = SolveSymmetricConstraint(tsum, m_Ty, num3);
			}
			T = tsum;
			Tr = T - m_inTd;
			float num6 = Mathf.Clamp(Fext.y * radius - m_inTd, 0f - m_inTb, m_inTb) + m_inTd;
			float num9;
			if (num3 <= 0f)
			{
				float num7 = Mathf.Max(m_Ty, num3);
				if (num6 < 0f)
				{
					float num8 = m_Ty * Mathf.InverseLerp(m_adherentSlip.y * Mathf.Sign(m_forwardSlip), 0f, m_forwardSlip);
					if (num6 < num8)
					{
						num6 = num8;
					}
					num9 = num6 - num7;
				}
				else
				{
					num9 = Mathf.Clamp(num6, 0f - num7, 0f - m_Ty);
				}
			}
			else
			{
				float num7 = Mathf.Min(m_Ty, num3);
				if (num6 > 0f)
				{
					float num10 = m_Ty * Mathf.InverseLerp(m_adherentSlip.y * Mathf.Sign(m_forwardSlip), 0f, m_forwardSlip);
					if (num6 > num10)
					{
						num6 = num10;
					}
					num9 = num6 - num7;
				}
				else
				{
					num9 = Mathf.Clamp(num6, 0f - m_Ty, 0f - num7);
				}
			}
			F.y = num9 / radius;
			float x;
			if (m_isAdherent)
			{
				m_adherentSlip.y = m_adherentSlip.y * F.y / m_adherentForce.y;
				m_contact.slip = m_adherentSlip;
				m_adherentSlip.x = tireFriction.GetAdherentSlipSideways(m_contact);
				m_contact.slip = m_adherentSlip;
				m_adherentForce = tireFriction.GetAdherentForce(m_contact);
				x = m_adherentForce.x;
			}
			else
			{
				x = m_tireForce.x;
			}
			x = MathUtility.FastAbs(x);
			F.x = Mathf.Clamp(Fext.x, 0f - x, x);
			isResting &= x > MathUtility.FastAbs(Fext.x);
			isResting &= m_inTb > MathUtility.FastAbs(Fext.y * radius - m_inTd);
		}

		public override void GetSubstepDerivative(ref Derivative D)
		{
			D.T = T;
			D.F = F;
			D.Tr = Tr;
		}

		public override void SetState(State S)
		{
			L = S.L;
			F = S.P / Solver.deltaTime;
			Tr = S.Lr / Solver.deltaTime;
			RecalculateVars();
			m_brakeTorque = 0f;
		}

		private static float SolveSymmetricConstraint(float Tsum, float T, float Tmax)
		{
			bool flag = Tsum >= 0f;
			bool flag2 = Tmax >= 0f;
			float num = (flag ? Tsum : (0f - Tsum));
			float num2 = (flag2 ? Tmax : (0f - Tmax));
			if (Tmax == 0f || (flag == flag2 && num > num2))
			{
				float t = Mathf.InverseLerp(0f, Tsum * 0.5f, T);
				Tsum = MathUtility.UnclampedLerp(Tsum, Tmax, t);
			}
			return Tsum;
		}
	}
}
