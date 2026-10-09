using UnityEngine;
using UnityEngine.EventSystems;

namespace KSP.UI
{
	[RequireComponent(typeof(EventTrigger))]
	public class EventTriggerForwarder : MonoBehaviour
	{
		public bool forwardDrop;

		public bool forwardPointerEnter;

		public bool forwardPointerExit;

		public bool forwardScroll;

		private void Awake()
		{
			EventTrigger component = GetComponent<EventTrigger>();
			if (forwardDrop)
			{
				component.AddEvent(EventTriggerType.Drop, Drop);
			}
			if (forwardScroll)
			{
				component.AddEvent(EventTriggerType.Scroll, Scroll);
			}
			if (forwardPointerEnter)
			{
				component.AddEvent(EventTriggerType.PointerEnter, PointerEnter);
			}
			if (forwardPointerExit)
			{
				component.AddEvent(EventTriggerType.PointerExit, PointerExit);
			}
		}

		private void Drop(BaseEventData data)
		{
			ExecuteEvents.ExecuteHierarchy(base.transform.parent.gameObject, data, delegate(IDropHandler x, BaseEventData y)
			{
				x.OnDrop(null);
			});
		}

		private void Scroll(BaseEventData data)
		{
			ExecuteEvents.ExecuteHierarchy(base.transform.parent.gameObject, data, delegate(IScrollHandler x, BaseEventData y)
			{
				x.OnScroll((PointerEventData)data);
			});
		}

		private void PointerEnter(BaseEventData data)
		{
			ExecuteEvents.ExecuteHierarchy(base.transform.parent.gameObject, data, delegate(IPointerEnterHandler x, BaseEventData y)
			{
				x.OnPointerEnter((PointerEventData)data);
			});
		}

		private void PointerExit(BaseEventData data)
		{
			ExecuteEvents.ExecuteHierarchy(base.transform.parent.gameObject, data, delegate(IPointerExitHandler x, BaseEventData y)
			{
				x.OnPointerExit((PointerEventData)data);
			});
		}
	}
}
