using System;
using UnityEngine;
using UnityEngine.UI;

namespace KSP.UI
{
	public class UIStateImage : MonoBehaviour
	{
		[Serializable]
		public class ImageState
		{
			public string name;

			public Sprite sprite;
		}

		public Image image;

		public ImageState[] states = new ImageState[0];

		public Sprite paused;

		public string startState = "";

		private void Reset()
		{
			image = GetComponent<Image>();
		}

		public void Awake()
		{
			GameEvents.onGamePause.Add(onGamePause);
			GameEvents.onGameUnpause.Add(onGameUnPause);
			SetState(startState);
		}

		private void OnDestroy()
		{
			GameEvents.onGamePause.Remove(onGamePause);
			GameEvents.onGameUnpause.Remove(onGameUnPause);
		}

		public void SetState(int index)
		{
			if (index >= 0 && index < states.Length && image != null)
			{
				image.sprite = states[index].sprite;
			}
		}

		public void SetState(string name)
		{
			int num = states.Length;
			while (num-- > 0)
			{
				if (states[num].name == name)
				{
					SetState(num);
				}
			}
		}

		private void onGamePause()
		{
			if (paused != null)
			{
				image.sprite = paused;
			}
		}

		private void onGameUnPause()
		{
			SetState(TimeWarp.CurrentRateIndex);
		}
	}
}
