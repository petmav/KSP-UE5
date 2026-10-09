using UnityEngine;

namespace KSP.UI
{
	public class UIPanelTransitionToggle : UIPanelTransition
	{
		[SerializeField]
		private int expandedIndex;

		[SerializeField]
		private int collapsedIndex = 1;

		public bool expanded => base.StateIndex == expandedIndex;

		public bool collapsed => base.StateIndex == collapsedIndex;

		protected void OnValidate()
		{
			if (states.Length > 2)
			{
				Debug.LogError("[UIPanelTransitionToggle]: Invalid Setup on " + base.name + "! Toggle type panels can only have 2 states!", base.gameObject);
			}
		}

		public void Expand()
		{
			Transition(expandedIndex);
		}

		public void Collapse()
		{
			Transition(collapsedIndex);
		}

		public void ExpandImmediate()
		{
			TransitionImmediate(expandedIndex);
		}

		public void CollapseImmediate()
		{
			TransitionImmediate(collapsedIndex);
		}
	}
}
