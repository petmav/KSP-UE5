using UnityEngine;

namespace KSP.UI.Screens.Editor
{
	public class PartIcon
	{
		private GameObject icon;

		private Renderer[] renderers;

		public GameObject Icon
		{
			get
			{
				return icon;
			}
			set
			{
				icon = value;
				if (icon != null)
				{
					renderers = icon.GetComponentsInChildren<Renderer>();
				}
			}
		}

		public Renderer[] Renderers => renderers;

		public PartIcon(GameObject newPartIcon)
		{
			Icon = newPartIcon;
		}

		public void DestroyIcon()
		{
			if (icon != null)
			{
				icon.DestroyGameObject();
			}
			renderers = null;
		}
	}
}
