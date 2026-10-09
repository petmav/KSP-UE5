using System;
using TMPro;
using UnityEngine;
using UnityEngine.UI;

namespace KSP.UI
{
	[ExecuteInEditMode]
	public class InteractableCtrlUIStates : MonoBehaviour
	{
		[Serializable]
		public class State
		{
			[SerializeField]
			private Graphic[] tgtGraphics;

			[SerializeField]
			private Color color = Color.white;

			[SerializeField]
			private bool toggleEnabled;

			[SerializeField]
			private TextMeshProUGUI tgtText;

			[SerializeField]
			private bool changeColorOnly;

			[SerializeField]
			private string text;

			public void SetActive()
			{
				int num = tgtGraphics.Length;
				while (num-- > 0)
				{
					tgtGraphics[num].color = color;
					if (toggleEnabled)
					{
						tgtGraphics[num].enabled = true;
					}
				}
				if (tgtText != null)
				{
					tgtText.color = color;
					if (!changeColorOnly)
					{
						tgtText.text = text;
					}
				}
			}
		}

		[SerializeField]
		private Selectable tgtCtrl;

		[SerializeField]
		private State stInteractable;

		[SerializeField]
		private State stNonInteractable;

		private bool ctrlFlagLast;

		private bool firstRun = true;

		private void Awake()
		{
		}

		private void LateUpdate()
		{
			if (ctrlFlagLast != tgtCtrl.interactable || firstRun)
			{
				ctrlFlagLast = tgtCtrl.interactable;
				if (ctrlFlagLast)
				{
					stInteractable.SetActive();
				}
				else
				{
					stNonInteractable.SetActive();
				}
			}
			if (firstRun)
			{
				firstRun = false;
			}
		}
	}
}
