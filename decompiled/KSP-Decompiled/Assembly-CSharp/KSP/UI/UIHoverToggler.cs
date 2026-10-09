using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.UI;

namespace KSP.UI
{
	public class UIHoverToggler : XSelectable
	{
		[SerializeField]
		private GameObject[] activeOnHover;

		[SerializeField]
		private Selectable tgtControl;

		[SerializeField]
		internal bool RequireInteractable = true;

		protected new void Start()
		{
			base.Start();
			base.onPointerEnter += OnHover;
			base.onPointerExit += OnUnhover;
			base.onDeselect += OnDeselect;
			OnUnhover(null, null);
		}

		protected new void OnDisable()
		{
			base.OnDisable();
			OnUnhover(null, null);
		}

		private void OnDeselect(XSelectable arg1, BaseEventData arg2)
		{
			OnUnhover(null, null);
		}

		protected void OnHover(XSelectable arg1, PointerEventData arg2)
		{
			if (tgtControl == null || !RequireInteractable || tgtControl.interactable)
			{
				int num = activeOnHover.Length;
				while (num-- > 0)
				{
					activeOnHover[num].SetActive(value: true);
				}
			}
		}

		protected void OnUnhover(XSelectable arg1, PointerEventData arg2)
		{
			int num = activeOnHover.Length;
			while (num-- > 0)
			{
				activeOnHover[num].SetActive(value: false);
			}
		}

		public void SetText(string text)
		{
			if (activeOnHover != null)
			{
				Text component = activeOnHover[0].GetComponent<Text>();
				if ((bool)component)
				{
					component.text = text;
				}
			}
		}
	}
}
