using KSP.UI.TooltipTypes;
using UnityEngine;
using UnityEngine.EventSystems;

namespace Expansions.Serenity
{
	public class AxisLimitLine : MonoBehaviour, IPointerClickHandler, IEventSystemHandler, IBeginDragHandler, IEndDragHandler, IDragHandler
	{
		public enum LimitOptions
		{
			Min,
			Max
		}

		[SerializeField]
		private LimitOptions limitType;

		[SerializeField]
		private TooltipController_Text toolTip;

		internal float deadZone = 1f;

		private Vector3 localPosCache;

		public RoboticControllerWindowAxis Axis { get; private set; }

		public LimitOptions LimitType => limitType;

		public void Setup(RoboticControllerWindowAxis axis, float deadzone, float initialValue)
		{
			Axis = axis;
			deadZone = deadzone;
			SetToolTipValue(initialValue);
		}

		private void Awake()
		{
			if (!ExpansionsLoader.IsExpansionInstalled("Serenity"))
			{
				Object.Destroy(base.gameObject);
			}
		}

		private void OnDestroy()
		{
		}

		public void OnPointerClick(PointerEventData eventData)
		{
		}

		public void OnBeginDrag(PointerEventData eventData)
		{
			Axis.BeginDragAxisLimit(this, eventData);
		}

		public void OnEndDrag(PointerEventData eventData)
		{
			Axis.EndDragAxisLimit(this, eventData);
		}

		public void OnDrag(PointerEventData eventData)
		{
			Axis.OnDragAxisLimit(this, eventData);
		}

		internal void SetToolTipValue(float newValue)
		{
			toolTip.SetText(newValue.ToString("F1"));
		}

		internal void UpdateYPosition(float newY)
		{
			localPosCache = base.transform.localPosition;
			localPosCache.y = newY + deadZone;
			base.transform.localPosition = localPosCache;
		}
	}
}
