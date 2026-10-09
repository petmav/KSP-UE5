using System;
using System.Runtime.CompilerServices;
using UnityEngine.Events;
using UnityEngine.EventSystems;

namespace UnityEngine.UI
{
	[RequireComponent(typeof(RectTransform))]
	[AddComponentMenu("UI/BoxSlider", 35)]
	public class BoxSlider : Selectable, IDragHandler, IEventSystemHandler, IInitializePotentialDragHandler, ICanvasElement
	{
		public enum Direction
		{
			LeftToRight,
			RightToLeft,
			BottomToTop,
			TopToBottom
		}

		[Serializable]
		public class BoxSliderEvent : UnityEvent<float, float>
		{
		}

		private enum Axis
		{
			Horizontal,
			Vertical
		}

		[SerializeField]
		private RectTransform m_HandleRect;

		[SerializeField]
		[Space(6f)]
		private float m_MinValue;

		[SerializeField]
		private float m_MaxValue = 1f;

		[SerializeField]
		private bool m_WholeNumbers;

		[SerializeField]
		private float m_Value = 1f;

		[SerializeField]
		private float m_ValueY = 1f;

		[SerializeField]
		[Space(6f)]
		private BoxSliderEvent m_OnValueChanged = new BoxSliderEvent();

		private Transform m_HandleTransform;

		private RectTransform m_HandleContainerRect;

		private Vector2 m_Offset = Vector2.zero;

		private DrivenRectTransformTracker m_Tracker;

		public RectTransform handleRect
		{
			get
			{
				return m_HandleRect;
			}
			set
			{
				if (SetClass(ref m_HandleRect, value))
				{
					UpdateCachedReferences();
					UpdateVisuals();
				}
			}
		}

		public float minValue
		{
			get
			{
				return m_MinValue;
			}
			set
			{
				if (SetStruct(ref m_MinValue, value))
				{
					Set(m_Value);
					SetY(m_ValueY);
					UpdateVisuals();
				}
			}
		}

		public float maxValue
		{
			get
			{
				return m_MaxValue;
			}
			set
			{
				if (SetStruct(ref m_MaxValue, value))
				{
					Set(m_Value);
					SetY(m_ValueY);
					UpdateVisuals();
				}
			}
		}

		public bool wholeNumbers
		{
			get
			{
				return m_WholeNumbers;
			}
			set
			{
				if (SetStruct(ref m_WholeNumbers, value))
				{
					Set(m_Value);
					SetY(m_ValueY);
					UpdateVisuals();
				}
			}
		}

		public float value
		{
			get
			{
				if (wholeNumbers)
				{
					return Mathf.Round(m_Value);
				}
				return m_Value;
			}
			set
			{
				Set(value);
			}
		}

		public float normalizedValue
		{
			get
			{
				if (Mathf.Approximately(minValue, maxValue))
				{
					return 0f;
				}
				return Mathf.InverseLerp(minValue, maxValue, value);
			}
			set
			{
				this.value = Mathf.Lerp(minValue, maxValue, value);
			}
		}

		public float valueY
		{
			get
			{
				if (wholeNumbers)
				{
					return Mathf.Round(m_ValueY);
				}
				return m_ValueY;
			}
			set
			{
				SetY(value);
			}
		}

		public float normalizedValueY
		{
			get
			{
				if (Mathf.Approximately(minValue, maxValue))
				{
					return 0f;
				}
				return Mathf.InverseLerp(minValue, maxValue, valueY);
			}
			set
			{
				valueY = Mathf.Lerp(minValue, maxValue, value);
			}
		}

		public BoxSliderEvent onValueChanged
		{
			get
			{
				return m_OnValueChanged;
			}
			set
			{
				m_OnValueChanged = value;
			}
		}

		private float stepSize
		{
			get
			{
				if (!wholeNumbers)
				{
					return (maxValue - minValue) * 0.1f;
				}
				return 1f;
			}
		}

		protected BoxSlider()
		{
		}

		public virtual void Rebuild(CanvasUpdate executing)
		{
		}

		public void LayoutComplete()
		{
		}

		public void GraphicUpdateComplete()
		{
		}

		public static bool SetClass<T>(ref T currentValue, T newValue) where T : class
		{
			if ((currentValue == null && newValue == null) || (currentValue != null && currentValue.Equals(newValue)))
			{
				return false;
			}
			currentValue = newValue;
			return true;
		}

		public static bool SetStruct<T>(ref T currentValue, T newValue) where T : struct
		{
			if (currentValue.Equals(newValue))
			{
				return false;
			}
			currentValue = newValue;
			return true;
		}

		protected override void OnEnable()
		{
			base.OnEnable();
			UpdateCachedReferences();
			Set(m_Value, sendCallback: false);
			SetY(m_ValueY, sendCallback: false);
			UpdateVisuals();
		}

		protected override void OnDisable()
		{
			m_Tracker.Clear();
			base.OnDisable();
		}

		private void UpdateCachedReferences()
		{
			if ((bool)m_HandleRect)
			{
				m_HandleTransform = m_HandleRect.transform;
				if (m_HandleTransform.parent != null)
				{
					m_HandleContainerRect = m_HandleTransform.parent.GetComponent<RectTransform>();
				}
			}
			else
			{
				m_HandleContainerRect = null;
			}
		}

		private void Set(float input)
		{
			Set(input, sendCallback: true);
		}

		private void Set(float input, bool sendCallback)
		{
			float num = Mathf.Clamp(input, minValue, maxValue);
			if (wholeNumbers)
			{
				num = Mathf.Round(num);
			}
			if (m_Value != num)
			{
				m_Value = num;
				UpdateVisuals();
				if (sendCallback)
				{
					m_OnValueChanged.Invoke(num, valueY);
				}
			}
		}

		private void SetY(float input)
		{
			SetY(input, sendCallback: true);
		}

		private void SetY(float input, bool sendCallback)
		{
			float num = Mathf.Clamp(input, minValue, maxValue);
			if (wholeNumbers)
			{
				num = Mathf.Round(num);
			}
			if (m_ValueY != num)
			{
				m_ValueY = num;
				UpdateVisuals();
				if (sendCallback)
				{
					m_OnValueChanged.Invoke(value, num);
				}
			}
		}

		protected override void OnRectTransformDimensionsChange()
		{
			base.OnRectTransformDimensionsChange();
			UpdateVisuals();
		}

		private void UpdateVisuals()
		{
			m_Tracker.Clear();
			if (m_HandleContainerRect != null)
			{
				m_Tracker.Add(this, m_HandleRect, DrivenTransformProperties.Anchors);
				Vector2 zero = Vector2.zero;
				Vector2 one = Vector2.one;
				float num2 = (one[0] = normalizedValue);
				zero[0] = num2;
				num2 = (one[1] = normalizedValueY);
				zero[1] = num2;
				m_HandleRect.anchorMin = zero;
				m_HandleRect.anchorMax = one;
			}
		}

		private void UpdateDrag(PointerEventData eventData, Camera cam)
		{
			RectTransform handleContainerRect = m_HandleContainerRect;
			if (handleContainerRect != null && handleContainerRect.rect.size[0] > 0f && RectTransformUtility.ScreenPointToLocalPointInRectangle(handleContainerRect, eventData.position, cam, out var localPoint))
			{
				localPoint -= handleContainerRect.rect.position;
				float num = Mathf.Clamp01((localPoint - m_Offset)[0] / handleContainerRect.rect.size[0]);
				normalizedValue = num;
				float num2 = Mathf.Clamp01((localPoint - m_Offset)[1] / handleContainerRect.rect.size[1]);
				normalizedValueY = num2;
			}
		}

		private bool MayDrag(PointerEventData eventData)
		{
			if (IsActive() && IsInteractable())
			{
				return eventData.button == PointerEventData.InputButton.Left;
			}
			return false;
		}

		public override void OnPointerDown(PointerEventData eventData)
		{
			if (!MayDrag(eventData))
			{
				return;
			}
			base.OnPointerDown(eventData);
			m_Offset = Vector2.zero;
			if (m_HandleContainerRect != null && RectTransformUtility.RectangleContainsScreenPoint(m_HandleRect, eventData.position, eventData.enterEventCamera))
			{
				if (RectTransformUtility.ScreenPointToLocalPointInRectangle(m_HandleRect, eventData.position, eventData.pressEventCamera, out var localPoint))
				{
					m_Offset = localPoint;
				}
				m_Offset.y = 0f - m_Offset.y;
			}
			else
			{
				UpdateDrag(eventData, eventData.pressEventCamera);
			}
		}

		public virtual void OnDrag(PointerEventData eventData)
		{
			if (MayDrag(eventData))
			{
				UpdateDrag(eventData, eventData.pressEventCamera);
			}
		}

		public virtual void OnInitializePotentialDrag(PointerEventData eventData)
		{
			eventData.useDragThreshold = false;
		}
	}
}
