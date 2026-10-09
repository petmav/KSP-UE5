using UnityEngine;
using UnityEngine.UI;

namespace KSP.UI.Screens
{
	[ExecuteInEditMode]
	public class HelixGauge : MonoBehaviour
	{
		public Image tgtImage;

		public float FillatMinValue;

		public float FillatMaxValue = 1f;

		public float MinValue;

		public float MaxValue = 1f;

		public float currentValue;

		public float currentAngle;

		public float angleAtMinValue = 1f;

		public float angleAtMaxValue = 1f;

		public bool showReadout;

		public RectTransform readoutField;

		public float readoutStandoff = 180f;

		[SerializeField]
		private float readoutStandOffPreScale;

		private void Awake()
		{
			currentValue = 0f;
			currentAngle = 0f;
		}

		private void Start()
		{
			UpdateGauge();
			if (readoutField == null)
			{
				showReadout = false;
			}
		}

		private void UpdateGauge()
		{
			float t = Mathf.InverseLerp(MinValue, MaxValue, currentValue);
			tgtImage.fillAmount = Mathf.Lerp(FillatMinValue, FillatMaxValue, t);
			currentAngle = Mathf.LerpAngle(angleAtMinValue, angleAtMaxValue, t);
		}

		protected void UpdateReadoutField()
		{
			float num = GameSettings.UI_SCALE;
			if (GameSettings.UIELEMENTSCALINGENABLED)
			{
				num *= GameSettings.UI_SCALE_NAVBALL;
			}
			readoutStandoff = num * readoutStandOffPreScale;
			readoutField.position = GetReadoutWorldPos(currentAngle, readoutStandoff, base.transform);
		}

		protected Vector3 GetReadoutWorldPos(float gaugeAngle, float spacing, Transform trf)
		{
			return trf.position + Quaternion.AngleAxis(gaugeAngle, -trf.forward) * trf.up * spacing;
		}

		private void LateUpdate()
		{
			UpdateGauge();
			if (showReadout)
			{
				UpdateReadoutField();
			}
			Debug.DrawRay(base.transform.position, Quaternion.AngleAxis(currentAngle, base.transform.forward) * base.transform.up, Color.green);
		}

		public float GetAngle(float v)
		{
			switch (tgtImage.fillMethod)
			{
			default:
				return 0f;
			case Image.FillMethod.Radial90:
				return GetAngle90(v);
			case Image.FillMethod.Radial180:
				return GetAngle180(v);
			case Image.FillMethod.Radial360:
				return GetAngle360(v);
			}
		}

		protected float GetAngle90(float v)
		{
			switch ((Image.Origin90)tgtImage.fillOrigin)
			{
			default:
				return 0f;
			case Image.Origin90.BottomLeft:
				if (tgtImage.fillClockwise)
				{
					return v * 90f;
				}
				return 90f - v * 90f;
			case Image.Origin90.TopLeft:
				if (tgtImage.fillClockwise)
				{
					return 90f + v * 90f;
				}
				return 180f - v * 90f;
			case Image.Origin90.TopRight:
				if (tgtImage.fillClockwise)
				{
					return 180f + v * 90f;
				}
				return 270f - v * 90f;
			case Image.Origin90.BottomRight:
				if (tgtImage.fillClockwise)
				{
					return 270f + v * 90f;
				}
				return 360f - v * 90f;
			}
		}

		protected float GetAngle180(float v)
		{
			switch ((Image.Origin180)tgtImage.fillOrigin)
			{
			default:
				return 0f;
			case Image.Origin180.Bottom:
				if (tgtImage.fillClockwise)
				{
					return -90f + v * 180f;
				}
				return 90f - v * 180f;
			case Image.Origin180.Left:
				if (tgtImage.fillClockwise)
				{
					return v * 180f;
				}
				return 180f - v * 180f;
			case Image.Origin180.Top:
				if (tgtImage.fillClockwise)
				{
					return 90f + v * 180f;
				}
				return 270f - v * 180f;
			case Image.Origin180.Right:
				if (tgtImage.fillClockwise)
				{
					return 180f + v * 180f;
				}
				return 360f - v * 180f;
			}
		}

		protected float GetAngle360(float v)
		{
			if (tgtImage.fillClockwise)
			{
				return v * 360f;
			}
			return 360f - v * 360f;
		}
	}
}
