using System;
using EdyCommonTools;
using UnityEngine;
using UnityEngine.Serialization;

namespace VehiclePhysics
{
	[Serializable]
	public class TireFriction : ISerializationCallbackReceiver
	{
		public enum Model
		{
			Flat,
			Lineal,
			Smooth,
			Parametric,
			Pacejka
		}

		[Serializable]
		public class Settings
		{
			public Vector2 adherent = new Vector2(0.5f, 0.95f);

			public Vector2 peak = new Vector2(1.5f, 1.1f);

			public Vector2 limit = new Vector2(4f, 0.8f);

			[Range(0f, 1f)]
			public float a;

			[Range(0f, 1f)]
			public float b;

			[Range(0f, 1f)]
			public float c = 0.5f;

			[Range(0f, 1f)]
			public float d;

			[Range(0.01f, 2f)]
			public float A = 0.95f;

			[Range(0.2f, 4f)]
			public float B = 0.8f;

			[Range(1f, 2f)]
			public float C = 1.5f;

			[Range(0f, 2f)]
			public float D = 1.1f;

			[Range(-20f, 1f)]
			public float E = -2f;

			public void ApplyAPLConstraints()
			{
				adherent.x = Mathf.Max(0.01f, adherent.x);
				peak.x = Mathf.Max(adherent.x, peak.x);
				limit.x = Mathf.Max(peak.x, limit.x);
				adherent.y = Mathf.Max(0f, adherent.y);
				peak.y = Mathf.Max(0f, peak.y);
				limit.y = Mathf.Max(0f, limit.y);
				adherent.y = Mathf.Min(peak.y, adherent.y);
				limit.y = Mathf.Min(peak.y, limit.y);
			}
		}

		public class ContactPatch
		{
			public Vector2 slip = Vector2.zero;

			public float load = 1f;

			public float groundGrip = 1f;
		}

		public class CurveBase
		{
			protected Settings m_params;

			public CurveBase(Settings settings)
			{
				m_params = settings;
			}

			public virtual float EvaluateForce(float slip, ContactPatch cp)
			{
				return 1f;
			}

			public virtual float GetAdherentSlip(ContactPatch cp)
			{
				return 0.5f;
			}

			public virtual float GetPeakSlip(ContactPatch cp)
			{
				return 1.5f;
			}

			public virtual float GetLimitSlip(ContactPatch cp)
			{
				return 4f;
			}
		}

		public class FlatFriction : CurveBase
		{
			public FlatFriction(Settings settings)
				: base(settings)
			{
				m_params.adherent.x = Mathf.Max(0.01f, m_params.adherent.x);
				m_params.adherent.y = Mathf.Max(0f, m_params.adherent.y);
			}

			public override float EvaluateForce(float s, ContactPatch cp)
			{
				return m_params.adherent.y * cp.load * cp.groundGrip;
			}

			public override float GetAdherentSlip(ContactPatch cp)
			{
				return m_params.adherent.x;
			}

			public override float GetPeakSlip(ContactPatch cp)
			{
				return m_params.adherent.x;
			}

			public override float GetLimitSlip(ContactPatch cp)
			{
				return m_params.adherent.x;
			}
		}

		public class LinealFriction : CurveBase
		{
			public LinealFriction(Settings settings)
				: base(settings)
			{
				settings.ApplyAPLConstraints();
			}

			private static float FastLerp(Vector2 P0, Vector2 P1, float x)
			{
				return P0.y + (x - P0.x) * (P1.y - P0.y) / (P1.x - P0.x);
			}

			public override float EvaluateForce(float s, ContactPatch cp)
			{
				float num = cp.load * cp.groundGrip;
				if (s <= m_params.adherent.x)
				{
					return num * m_params.adherent.y;
				}
				if (s < m_params.peak.x)
				{
					return num * FastLerp(m_params.adherent, m_params.peak, s);
				}
				if (s < m_params.limit.x)
				{
					return num * FastLerp(m_params.peak, m_params.limit, s);
				}
				return num * m_params.limit.y;
			}

			public override float GetAdherentSlip(ContactPatch cp)
			{
				return m_params.adherent.x;
			}

			public override float GetPeakSlip(ContactPatch cp)
			{
				return m_params.peak.x;
			}

			public override float GetLimitSlip(ContactPatch cp)
			{
				return m_params.limit.x;
			}
		}

		public class SmoothFriction : CurveBase
		{
			public SmoothFriction(Settings settings)
				: base(settings)
			{
				settings.ApplyAPLConstraints();
			}

			private static float CubicLerp(float x, Vector2 P0, Vector2 P1)
			{
				float num = (x - P0.x) / (P1.x - P0.x);
				float num2 = num * num;
				float num3 = num * num2;
				return P0.y * (2f * num3 - 3f * num2 + 1f) + P1.y * (-2f * num3 + 3f * num2);
			}

			public override float EvaluateForce(float s, ContactPatch cp)
			{
				float num = cp.load * cp.groundGrip;
				if (s <= m_params.adherent.x)
				{
					return num * m_params.adherent.y;
				}
				if (s < m_params.peak.x)
				{
					return num * CubicLerp(s, m_params.adherent, m_params.peak);
				}
				if (s < m_params.limit.x)
				{
					return num * CubicLerp(s, m_params.peak, m_params.limit);
				}
				return num * m_params.limit.y;
			}

			public override float GetAdherentSlip(ContactPatch cp)
			{
				return m_params.adherent.x;
			}

			public override float GetPeakSlip(ContactPatch cp)
			{
				return m_params.peak.x;
			}

			public override float GetLimitSlip(ContactPatch cp)
			{
				return m_params.limit.x;
			}
		}

		public class ParametricFriction : CurveBase
		{
			public ParametricFriction(Settings settings)
				: base(settings)
			{
				settings.ApplyAPLConstraints();
			}

			private static float TangentLerp(float x, Vector2 P0, Vector2 P1, float a, float b)
			{
				float num = P1.y - P0.y;
				float num2 = 3f * num * a;
				float num3 = 3f * num * b;
				float num4 = (x - P0.x) / (P1.x - P0.x);
				float num5 = num4 * num4;
				float num6 = num4 * num5;
				return P0.y * (2f * num6 - 3f * num5 + 1f) + P1.y * (-2f * num6 + 3f * num5) + num2 * (num6 - 2f * num5 + num4) + num3 * (num6 - num5);
			}

			public override float EvaluateForce(float s, ContactPatch cp)
			{
				float num = cp.load * cp.groundGrip;
				if (s <= m_params.adherent.x)
				{
					return num * m_params.adherent.y;
				}
				if (s < m_params.peak.x)
				{
					return num * TangentLerp(s, m_params.adherent, m_params.peak, m_params.a, m_params.b);
				}
				if (s < m_params.limit.x)
				{
					return num * TangentLerp(s, m_params.peak, m_params.limit, m_params.c, m_params.d);
				}
				return num * m_params.limit.y;
			}

			public override float GetAdherentSlip(ContactPatch cp)
			{
				return m_params.adherent.x;
			}

			public override float GetPeakSlip(ContactPatch cp)
			{
				return m_params.peak.x;
			}

			public override float GetLimitSlip(ContactPatch cp)
			{
				return m_params.limit.x;
			}
		}

		public class PacejkaFriction : CurveBase
		{
			private Vector2 m_adherent = Vector2.zero;

			private Vector2 m_peak = Vector2.zero;

			private Vector2 m_limit = Vector2.zero;

			public PacejkaFriction(Settings settings)
				: base(settings)
			{
				settings.A = Mathf.Max(settings.A, 0.01f);
				settings.D = Mathf.Max(settings.D, 0f);
				settings.B = Mathf.Max(settings.B, 0.1f);
				settings.C = Mathf.Clamp(settings.C, 1f, 2f);
				settings.E = Mathf.Min(settings.E, 1f);
				GetPacejkaPoints(settings.A, settings.B, settings.C, settings.D, settings.E, ref m_adherent, ref m_peak, ref m_limit);
			}

			public static void GetPacejkaPoints(float A, float B, float C, float D, float E, ref Vector2 adherent, ref Vector2 peak, ref Vector2 limit)
			{
				bool flag = false;
				bool flag2 = false;
				bool flag3 = false;
				float num = 0f;
				float num2 = 0.01f;
				float num3 = 15f;
				float num4 = Pacejka(num, B, C, D, E);
				while (num < num3)
				{
					float num5 = Pacejka(num + num2, B, C, D, E);
					if (!flag && A >= num4 && A < num5)
					{
						adherent.x = Mathf.Lerp(num, num + num2, Mathf.InverseLerp(num4, num5, A));
						adherent.y = Pacejka(adherent.x, B, C, D, E);
						flag = true;
					}
					if (!flag2 && num5 <= num4)
					{
						peak.x = num;
						peak.y = num4;
						flag2 = true;
					}
					if (!flag2 || !(num > peak.x + 1f / B) || MathUtility.FastAbs(num5 - num4) / num2 >= 0.01f)
					{
						num += num2;
						num4 = num5;
						continue;
					}
					limit.x = num + num2;
					flag3 = true;
					break;
				}
				if (!flag3)
				{
					limit.x = num3;
				}
				limit.y = Pacejka(limit.x, B, C, D, E);
				if (!flag2)
				{
					peak = limit;
				}
				if (!flag)
				{
					adherent = (flag2 ? peak : limit);
				}
			}

			public static float Pacejka(float slip, float B, float C, float D, float E)
			{
				float num = B * slip;
				return D * Mathf.Sin(C * Mathf.Atan(num - E * (num - Mathf.Atan(num))));
			}

			public override float EvaluateForce(float s, ContactPatch cp)
			{
				float num = cp.load * cp.groundGrip;
				if (s <= m_adherent.x)
				{
					return num * m_adherent.y;
				}
				if (s > m_limit.x)
				{
					return num * m_limit.y;
				}
				return num * Pacejka(s, m_params.B, m_params.C, m_params.D, m_params.E);
			}

			public override float GetAdherentSlip(ContactPatch cp)
			{
				return m_adherent.x;
			}

			public override float GetPeakSlip(ContactPatch cp)
			{
				return m_peak.x;
			}

			public override float GetLimitSlip(ContactPatch cp)
			{
				return m_limit.x;
			}
		}

		[FormerlySerializedAs("type")]
		public Model model = Model.Smooth;

		public Settings settings = new Settings();

		public float frictionMultiplier = 1f;

		private CurveBase m_curve;

		public TireFriction()
		{
			SetupFrictionCurves();
		}

		public void OnBeforeSerialize()
		{
		}

		public void OnAfterDeserialize()
		{
			SetupFrictionCurves();
		}

		public void SetupFrictionCurves()
		{
			switch (model)
			{
			case Model.Flat:
				m_curve = new FlatFriction(settings);
				break;
			case Model.Lineal:
				m_curve = new LinealFriction(settings);
				break;
			case Model.Smooth:
				m_curve = new SmoothFriction(settings);
				break;
			case Model.Parametric:
				m_curve = new ParametricFriction(settings);
				break;
			case Model.Pacejka:
				m_curve = new PacejkaFriction(settings);
				break;
			}
		}

		public Vector2 GetForce(ContactPatch cp)
		{
			Vector2 slip = cp.slip;
			float magnitude = slip.magnitude;
			if (magnitude < 1E-06f)
			{
				slip.x = 0f;
				slip.y = 0f;
				return slip;
			}
			slip.x /= magnitude;
			slip.y /= magnitude;
			return (0f - m_curve.EvaluateForce(magnitude, cp)) * slip * frictionMultiplier;
		}

		public bool IsAdherentSlip(ContactPatch cp)
		{
			return cp.slip.magnitude < m_curve.GetAdherentSlip(cp);
		}

		public float GetAdherentSlipForward(ContactPatch cp)
		{
			float x = cp.slip.x;
			float adherentSlip = m_curve.GetAdherentSlip(cp);
			if (!(MathUtility.FastAbs(x) < adherentSlip))
			{
				return 0f;
			}
			return Mathf.Sqrt(adherentSlip * adherentSlip - x * x);
		}

		public float GetAdherentSlipSideways(ContactPatch cp)
		{
			float y = cp.slip.y;
			float adherentSlip = m_curve.GetAdherentSlip(cp);
			if (!(MathUtility.FastAbs(y) < adherentSlip))
			{
				return 0f;
			}
			return Mathf.Sqrt(adherentSlip * adherentSlip - y * y);
		}

		public Vector2 GetAdherentSlipBounds(ContactPatch cp)
		{
			float adherentSlip = m_curve.GetAdherentSlip(cp);
			return new Vector2(adherentSlip, adherentSlip);
		}

		public Vector2 GetPeakSlipBounds(ContactPatch cp)
		{
			float peakSlip = m_curve.GetPeakSlip(cp);
			return new Vector2(peakSlip, peakSlip);
		}

		public Vector2 GetLimitSlipBounds(ContactPatch cp)
		{
			float limitSlip = m_curve.GetLimitSlip(cp);
			return new Vector2(limitSlip, limitSlip);
		}

		public Vector2 GetAdherentForce(ContactPatch cp)
		{
			float adherentSlip = m_curve.GetAdherentSlip(cp);
			return (0f - m_curve.EvaluateForce(adherentSlip, cp) * frictionMultiplier / adherentSlip) * cp.slip;
		}
	}
}
