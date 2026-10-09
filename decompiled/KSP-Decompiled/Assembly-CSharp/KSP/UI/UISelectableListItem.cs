using UnityEngine;

namespace KSP.UI
{
	public class UISelectableListItem : MonoBehaviour
	{
		private int index;

		public int Index => index;

		public virtual void Select()
		{
		}

		public virtual void Deselect()
		{
		}

		public virtual void SetupListItem(UISelectableList selectableList, int index)
		{
			this.index = index;
		}
	}
}
