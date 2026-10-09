using UnityEngine;

namespace KSP.UI.Screens.Flight
{
	public class MapViewPanelTransition : MonoBehaviour
	{
		public bool inDuringMap = true;

		public UIPanelTransition panel;

		private void Reset()
		{
			panel = GetComponent<UIPanelTransition>();
		}

		private void Awake()
		{
			GameEvents.OnMapEntered.Add(SetModeMapOn);
			GameEvents.OnMapExited.Add(SetModeMapOff);
			if (panel == null)
			{
				panel = GetComponent<UIPanelTransition>();
			}
		}

		private void OnDestroy()
		{
			GameEvents.OnMapEntered.Remove(SetModeMapOn);
			GameEvents.OnMapExited.Remove(SetModeMapOff);
		}

		public void SetModeMapOn()
		{
			panel.Transition(inDuringMap ? "In" : "Out");
		}

		public void SetModeMapOff()
		{
			panel.Transition((!inDuringMap) ? "In" : "Out");
		}
	}
}
