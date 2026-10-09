using System.Collections;
using UnityEngine;
using UnityEngine.UI;

namespace KSP.UI
{
	[RequireComponent(typeof(LayoutElement))]
	public class ResizingLayoutElement : MonoBehaviour
	{
		private enum Direction
		{
			VERTICAL,
			HORIZONTAL,
			BOTH
		}

		[HideInInspector]
		public object data;

		[HideInInspector]
		public int dataInt;

		private LayoutElement layoutElement;

		private RectTransform rectTransform;

		private float startWidth;

		private float startHeight;

		private float endWidth;

		private float endHeight;

		private float duration;

		private bool destroyOnCompletion;

		private Callback onComplete = delegate
		{
		};

		private Direction direction;

		private float currentHeight = -1f;

		private float currentWidth = -1f;

		private bool isReversing;

		private bool isResizing;

		public bool IsResizing => isResizing;

		private void Awake()
		{
			layoutElement = GetComponent<LayoutElement>();
			rectTransform = GetComponent<RectTransform>();
			rectTransform.pivot = new Vector2(0f, 1f);
		}

		public void StartResizing(float startHeight, float endHeight, float duration, bool destroyOnCompletion, Callback onComplete)
		{
			direction = Direction.VERTICAL;
			this.startHeight = startHeight;
			this.endHeight = endHeight;
			this.duration = duration;
			this.destroyOnCompletion = destroyOnCompletion;
			this.onComplete = onComplete;
			layoutElement.preferredHeight = startHeight;
			StartCoroutine(Resize());
		}

		public void ReverseResizing(bool destroyOnCompletion, Callback onComplete)
		{
			if (!isReversing)
			{
				isReversing = true;
				if (IsResizing)
				{
					isResizing = false;
					this.onComplete();
					StopAllCoroutines();
				}
				endHeight = startHeight;
				startHeight = currentHeight;
				this.destroyOnCompletion = destroyOnCompletion;
				this.onComplete = onComplete;
				StartCoroutine(Resize());
			}
		}

		private IEnumerator Resize()
		{
			float realtimeSinceStartup = Time.realtimeSinceStartup;
			float timeAtLastFrame = realtimeSinceStartup;
			isResizing = true;
			float time = 0f;
			while (time < duration)
			{
				realtimeSinceStartup = Time.realtimeSinceStartup;
				float num = realtimeSinceStartup - timeAtLastFrame;
				timeAtLastFrame = realtimeSinceStartup;
				time += num;
				if (direction != 0)
				{
					currentWidth = Mathf.Lerp(startWidth, endWidth, time / duration);
					layoutElement.preferredWidth = currentWidth;
				}
				if (direction != Direction.HORIZONTAL)
				{
					currentHeight = Mathf.Lerp(startHeight, endHeight, time / duration);
					layoutElement.preferredHeight = currentHeight;
				}
				yield return null;
			}
			isResizing = false;
			onComplete();
			if (destroyOnCompletion)
			{
				Object.Destroy(base.gameObject);
			}
		}
	}
}
