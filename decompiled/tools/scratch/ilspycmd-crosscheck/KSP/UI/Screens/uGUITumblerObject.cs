using System;
using TMPro;
using UnityEngine;

namespace KSP.UI.Screens
{
	public class uGUITumblerObject : MonoBehaviour
	{
		[SerializeField]
		private RectTransform[] Labels;

		[SerializeField]
		private string[] Values;

		public double tgtRot;

		public double currRot;

		public double N;

		public void TumbleTo(double n, int tumble)
		{
			n = Math.Floor(n % 10.0);
			tgtRot = 36.0 * n;
			switch (tumble)
			{
			case 1:
				if (n > N)
				{
					currRot += 360.0;
				}
				break;
			case 0:
				if (n < N)
				{
					currRot -= 360.0;
				}
				break;
			}
			N = n;
		}

		public void UpdateDelta(float deltaTime, float sharpness)
		{
			if ((double)(sharpness * deltaTime) > 1.0)
			{
				currRot = tgtRot;
			}
			else
			{
				currRot += (tgtRot - currRot) * (double)(sharpness * deltaTime);
			}
			int num = Mathf.FloorToInt((float)currRot / 36f);
			double num2 = currRot - (double)((float)num * 36f);
			for (int i = 0; i < Labels.Length; i++)
			{
				Labels[i].anchoredPosition = new Vector2(0f, (float)(0.0 - num2) * 23f / 36f - (float)((i - 2) * 23));
				Labels[i].GetComponent<TextMeshProUGUI>().text = GetValue(num + 2 - i);
			}
		}

		protected string GetValue(int index)
		{
			index = ((index < 0) ? ((index % Values.Length != 0) ? (Values.Length + index % Values.Length) : 0) : (index % Values.Length));
			return Values[index];
		}
	}
}
