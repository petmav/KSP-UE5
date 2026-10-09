using UnityEngine;

namespace KSP.UI.Screens
{
	public class SpriteTumblerObject : MonoBehaviour
	{
		public delegate void afterAltModeTumbleTo(double n, int tumble);

		public delegate void afterAltModeUpdateDelta(float deltaTime, float sharpness);

		public RectTransform[] images;

		public Sprite[] sprites;

		public RectTransform colorBar;

		public double tgtRot;

		public double currRot = 36.0;

		public double N;

		public bool tumbling;

		public static afterAltModeTumbleTo AfterAltTumbleTo;

		public static afterAltModeUpdateDelta AfterAltModeUpdateDelta;

		[SerializeField]
		private double delta;

		[SerializeField]
		private int index;

		[SerializeField]
		private int spriteIndexOffset;

		[SerializeField]
		private float colorBarMinY = 9f;

		[SerializeField]
		private float colorBarMaxY = 35f;

		public float colorBarTarget;

		public float colorBarCurrent;

		public float colorBarSection = 28f;

		public float IconSection = 36f;

		private void Awake()
		{
			N = 0.0;
			tgtRot = 0.0;
			currRot = tgtRot;
			colorBarCurrent = 33f;
			for (int i = 0; i < images.Length; i++)
			{
				images[i].anchoredPosition = new Vector2(0f, (float)N - (float)((i - spriteIndexOffset) * 23));
			}
			if (colorBar != null)
			{
				colorBar.anchoredPosition = new Vector2(colorBar.anchoredPosition.x, Mathf.Clamp(colorBarCurrent, colorBarMinY, colorBarMaxY));
			}
		}

		public void TumbleTo(double n, int tumble)
		{
			if (n != N)
			{
				tgtRot = (double)IconSection * n;
				colorBarTarget = colorBarSection * (float)n * 2f - (float)n;
				N = n;
				tumbling = true;
				if (AfterAltTumbleTo != null)
				{
					AfterAltTumbleTo(n, tumble);
				}
			}
		}

		public void UpdateDelta(float deltaTime, float sharpness)
		{
			if ((double)(sharpness * deltaTime) > 1.0)
			{
				currRot = tgtRot;
				colorBarCurrent = colorBarTarget;
			}
			else
			{
				currRot += (tgtRot - currRot) * (double)(sharpness * deltaTime);
				colorBarCurrent += (colorBarTarget - colorBarCurrent) * (sharpness * deltaTime);
			}
			index = Mathf.FloorToInt((float)currRot / IconSection);
			delta = currRot - (double)((float)index * IconSection);
			if (tumbling)
			{
				for (int i = 0; i < images.Length; i++)
				{
					images[i].anchoredPosition = new Vector2(0f, (float)delta * 23f / IconSection - (float)((i - spriteIndexOffset) * 23) + (float)index * 23f);
				}
			}
			if (colorBar != null)
			{
				colorBar.anchoredPosition = new Vector2(colorBar.anchoredPosition.x, Mathf.Clamp(colorBarCurrent, colorBarMinY, colorBarMaxY));
			}
			if (AfterAltModeUpdateDelta != null)
			{
				AfterAltModeUpdateDelta(deltaTime, sharpness);
			}
			if (delta > (double)(IconSection - 0.01f) || delta < 0.009999999776482582)
			{
				tumbling = false;
			}
		}
	}
}
