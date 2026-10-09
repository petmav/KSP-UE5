using System;
using KSP.UI.Util;
using UnityEngine;
using UnityEngine.UI;

namespace KSP.UI.Screens.Flight
{
	public class TemperatureGauge : MonoBehaviour, IComparable<TemperatureGauge>
	{
		[SerializeField]
		private Slider progressBar;

		[SerializeField]
		private Graphic sliderFill;

		public RectTransform rTrf;

		private Vector3 scPos;

		private Vector3 uiPos;

		public static float zSpaceEasing = 0.8f;

		public static float zSpaceMidpoint = 20f;

		public static float zSpaceLength = 100f;

		protected Part part;

		protected float distanceFromCamera;

		protected float zOffset;

		public float edgeHighlightThreshold = 0.5f;

		public float gaugeThreshold = 0.7f;

		private float colorScale;

		private float edgeRatio;

		public bool highlightActive;

		public bool gaugeActive;

		public bool showGauge;

		public Part Part
		{
			get
			{
				return part;
			}
			set
			{
				part = value;
			}
		}

		public float DistanceFromCamera => distanceFromCamera;

		public float ZOffset
		{
			get
			{
				return zOffset;
			}
			set
			{
				zOffset = value;
			}
		}

		protected virtual void Awake()
		{
			gaugeActive = true;
			rTrf = GetComponent<RectTransform>();
		}

		public virtual void Setup(Part p, float edgeHighlightThreshold, float gaugeThreshold)
		{
			this.edgeHighlightThreshold = edgeHighlightThreshold;
			this.gaugeThreshold = gaugeThreshold;
			part = p;
			highlightActive = false;
		}

		protected virtual void OnDestroy()
		{
			try
			{
				if (progressBar != null && progressBar.gameObject != null)
				{
					progressBar.gameObject.SetActive(value: false);
				}
				if (part != null)
				{
					part.SetHighlightDefault();
				}
			}
			catch (UnityException ex)
			{
				Debug.LogWarning("Gauge OnDestroy caught exception " + ex);
			}
			gaugeActive = false;
			highlightActive = false;
		}

		public virtual void GaugeUpdate()
		{
			if (part == null)
			{
				return;
			}
			float num = (float)(part.skinTemperature / part.skinMaxTemp);
			float num2 = (float)(part.temperature / part.maxTemp);
			if (num2 > num)
			{
				num = num2;
			}
			num = Mathf.Clamp01(num);
			if ((GameSettings.TEMPERATURE_GAUGES_MODE & 1) > 0 && num > gaugeThreshold * part.gaugeThresholdMult)
			{
				distanceFromCamera = Vector3.SqrMagnitude(FlightCamera.fetch.mainCamera.transform.position - part.partTransform.position);
				uiPos = RectUtil.WorldToUISpacePos(part.partTransform.position, FlightCamera.fetch.mainCamera, MainCanvasUtil.MainCanvasRect, ref showGauge);
			}
			else
			{
				showGauge = false;
			}
			if (showGauge)
			{
				rTrf.localPosition = uiPos;
				sliderFill.color = Color.Lerp(Color.green, Color.red, num);
				progressBar.value = num;
				if (!gaugeActive)
				{
					progressBar.gameObject.SetActive(value: true);
					gaugeActive = true;
				}
			}
			if (!showGauge && gaugeActive)
			{
				progressBar.gameObject.SetActive(value: false);
				gaugeActive = false;
			}
			if ((GameSettings.TEMPERATURE_GAUGES_MODE & 2) > 0 && num > edgeHighlightThreshold * part.edgeHighlightThresholdMult)
			{
				if (!highlightActive)
				{
					highlightActive = true;
					part.SetHighlightType(Part.HighlightType.AlwaysOn);
					part.SetHighlight(active: true, recursive: false);
				}
				edgeRatio = Mathf.InverseLerp(edgeHighlightThreshold * part.edgeHighlightThresholdMult, 1f, num);
				colorScale = Mathf.Clamp01(num * 1f);
				part.SetHighlightColor(Color.Lerp(XKCDColors.Red * colorScale, XKCDColors.KSPNotSoGoodOrange * colorScale, edgeRatio));
			}
			else if (highlightActive)
			{
				highlightActive = false;
				part.SetHighlightDefault();
			}
		}

		public int CompareTo(TemperatureGauge b)
		{
			if (distanceFromCamera < b.distanceFromCamera)
			{
				return -1;
			}
			if (distanceFromCamera == b.distanceFromCamera)
			{
				return 0;
			}
			return 1;
		}
	}
}
