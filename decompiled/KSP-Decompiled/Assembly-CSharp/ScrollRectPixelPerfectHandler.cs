using UnityEngine;
using UnityEngine.UI;

public class ScrollRectPixelPerfectHandler : MonoBehaviour
{
	public ScrollRect scrollRect;

	private Scrollbar horizontal;

	private Scrollbar vertical;

	private CanvasPixelPerfectHandler pixelPerfectHandler;

	private void Start()
	{
		pixelPerfectHandler = base.gameObject.GetComponentUpwards<CanvasPixelPerfectHandler>();
		if (!(pixelPerfectHandler == null) && !(scrollRect == null))
		{
			horizontal = scrollRect.horizontalScrollbar;
			vertical = scrollRect.verticalScrollbar;
			if (horizontal != null)
			{
				horizontal.onValueChanged.AddListener(DisablePixelPerfect);
			}
			if (vertical != null)
			{
				vertical.onValueChanged.AddListener(DisablePixelPerfect);
			}
		}
		else
		{
			Object.Destroy(this);
		}
	}

	private void Update()
	{
		if (scrollRect != null && scrollRect.velocity.sqrMagnitude > 0f)
		{
			DisablePixelPerfect();
		}
	}

	private void DisablePixelPerfect(float delta = 0f)
	{
		if ((bool)pixelPerfectHandler)
		{
			pixelPerfectHandler.TemporaryDisable();
		}
	}
}
