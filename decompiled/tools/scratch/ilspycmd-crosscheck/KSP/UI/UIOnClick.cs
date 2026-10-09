using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.UI;

namespace KSP.UI
{
	public class UIOnClick : MonoBehaviour, IPointerClickHandler, IEventSystemHandler
	{
		public EventData<PointerEventData> onClick = new EventData<PointerEventData>("OnClick");

		private Selectable selectable;

		public bool interactable = true;

		public bool Interactable
		{
			get
			{
				if (selectable != null)
				{
					return selectable.interactable;
				}
				return interactable;
			}
			set
			{
				if (selectable != null)
				{
					selectable.interactable = value;
				}
				interactable = value;
			}
		}

		private void Awake()
		{
			selectable = GetComponent<Selectable>();
		}

		public void OnPointerClick(PointerEventData eventData)
		{
			if (Interactable)
			{
				onClick.Fire(eventData);
			}
		}
	}
}
