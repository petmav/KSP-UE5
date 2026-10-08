using TMPro;
using UnityEngine;

namespace KSP.UI.Screens
{
	[ExecuteInEditMode]
	public class RotationalGaugeOffsetMarker : RotationalGauge
	{
		[SerializeField]
		private RectTransform offsetPointer;

		[SerializeField]
		private float nextToCurrentAngle;

		[SerializeField]
		private float offsetAngle;

		[SerializeField]
		private TextMeshProUGUI offsetTextField;

		[SerializeField]
		private GameObject stageMarker;

		[SerializeField]
		internal GameObject offsetObject;

		[SerializeField]
		private GameObject offsetGameObjectPrefab;

		[SerializeField]
		private double stageDV;

		public double StageDV
		{
			get
			{
				return stageDV;
			}
			set
			{
				stageDV = value;
			}
		}

		protected override void Awake()
		{
			base.Awake();
			offsetObject = Object.Instantiate(offsetGameObjectPrefab);
			offsetTextField = offsetObject.GetComponentInChildren<TextMeshProUGUI>();
			offsetPointer = offsetObject.GetComponent<RectTransform>();
		}

		protected void Start()
		{
			offsetObject.transform.localScale = Vector3.one;
			offsetObject.transform.localPosition = Vector3.zero;
			offsetObject.transform.SetParent(base.gameObject.transform.parent);
		}

		protected void OnDestroy()
		{
			if (offsetObject != null)
			{
				offsetObject.DestroyGameObject();
			}
		}

		protected override void LateUpdate()
		{
			base.LateUpdate();
			offsetAngle = currentAngle + (nextToCurrentAngle - currentAngle) / 2f;
			offsetPointer.localRotation = Quaternion.AngleAxis(offsetAngle, rotationAxis);
		}

		public void SetNextToValue(float nextAngle)
		{
			nextToCurrentAngle = nextAngle;
		}

		public void SetTextField(string text, int opacity)
		{
			opacity = Mathf.Clamp(opacity, 25, 255);
			offsetTextField.text = string.Format("<color=#FFFFFF{1:X}>{0}</color>", text, opacity);
		}

		public void ToggleOffsetMarker(bool active)
		{
			stageMarker.SetActive(active);
		}
	}
}
