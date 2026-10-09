using System.Collections;
using UnityEngine;
using UnityEngine.EventSystems;

namespace KSP.UI
{
	public class PointerClickAndHoldHandler : PointerClickHandler, IPointerExitHandler, IEventSystemHandler
	{
		public PointerClickEvent<PointerEventData> onPointerDownHold = new PointerClickEvent<PointerEventData>();

		private Coroutine pointerRoutine;

		public override void OnPointerDown(PointerEventData eventData)
		{
			onPointerDown.Invoke(eventData);
			pointerRoutine = StartCoroutine(OnPointerDownHold(eventData));
		}

		public override void OnPointerUp(PointerEventData eventData)
		{
			if (pointerRoutine != null)
			{
				onPointerUp.Invoke(eventData);
				StopCoroutine(pointerRoutine);
			}
		}

		public void OnPointerExit(PointerEventData eventData)
		{
			if (pointerRoutine != null)
			{
				StopCoroutine(pointerRoutine);
			}
		}

		private IEnumerator OnPointerDownHold(PointerEventData eventData)
		{
			while (true)
			{
				onPointerDownHold.Invoke(eventData);
				yield return null;
			}
		}
	}
}
