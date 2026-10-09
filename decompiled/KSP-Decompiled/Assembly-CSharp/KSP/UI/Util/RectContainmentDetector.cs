using UnityEngine;

namespace KSP.UI.Util
{
	public class RectContainmentDetector : MonoBehaviour
	{
		public RectTransform container;

		public RectTransform refRect;

		public bool twoWayTest;

		private RectUtil.ContainmentLevel level;

		private RectUtil.ContainmentLevel levelLast;

		private bool initialUpdate;

		[SerializeField]
		private Camera refCamera;

		public RectUtil.ContainmentLevel Level => level;

		public event Callback<RectUtil.ContainmentLevel> OnContainmentChanged = delegate
		{
		};

		protected void Start()
		{
			refCamera = UIMainCamera.Camera;
			if (refRect == null)
			{
				refRect = GetComponent<RectTransform>();
			}
			level = RectUtil.ContainmentLevel.Full;
			levelLast = RectUtil.ContainmentLevel.None;
			initialUpdate = false;
		}

		protected void Update()
		{
			level = RectUtil.GetRectContainment(refRect, container, refCamera, twoWayTest);
			if (level != levelLast || !initialUpdate)
			{
				this.OnContainmentChanged(level);
				levelLast = level;
				initialUpdate = true;
			}
		}
	}
}
