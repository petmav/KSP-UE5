using System.Runtime.CompilerServices;
using UnityEngine.EventSystems;

namespace KSP.UI
{
	public class PinnableTooltipController : TooltipController, IPinnableTooltipController, ITooltipController, IPointerClickHandler, IEventSystemHandler
	{
		protected bool pinned;

		public virtual void OnPointerClick(PointerEventData eventData)
		{
			if (eventData.button == PointerEventData.InputButton.Right)
			{
				if (!pinned)
				{
					UIMasterController.Instance.PinTooltip(this);
					pinned = true;
				}
				else
				{
					UIMasterController.Instance.UnpinTooltip(this);
					pinned = false;
				}
			}
		}

		public virtual void OnTooltipPinned()
		{
		}

		public virtual void OnTooltipUnpinned()
		{
		}

		public bool IsPinned()
		{
			return pinned;
		}

		public void Unpin()
		{
			pinned = false;
		}

		public override void OnPointerEnter(PointerEventData eventData)
		{
			if (UIMasterController.Instance != null)
			{
				UIMasterController.Instance.SpawnTooltip(this);
			}
		}

		public override void OnPointerExit(PointerEventData eventData)
		{
			if (!pinned)
			{
				OnDestroy();
			}
		}

		[SpecialName]
		string ITooltipController.get_name()
		{
			return base.name;
		}

		[SpecialName]
		void ITooltipController.set_name(string value)
		{
			base.name = value;
		}
	}
}
