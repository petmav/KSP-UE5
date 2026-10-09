using System;
using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.Events;

namespace KSP.UI
{
	public class DropHandler : MonoBehaviour, IDropHandler, IEventSystemHandler
	{
		[Serializable]
		public class DropEvent<PointerEventData> : UnityEvent<PointerEventData>
		{
		}

		public DropEvent<PointerEventData> onDrop = new DropEvent<PointerEventData>();

		public virtual void OnDrop(PointerEventData eventData)
		{
			onDrop.Invoke(eventData);
		}
	}
}
