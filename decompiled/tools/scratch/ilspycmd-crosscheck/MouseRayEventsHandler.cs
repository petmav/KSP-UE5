using UnityEngine;

public class MouseRayEventsHandler : MouseEventsHandlerBase
{
	private const int trackDelayFrames = 5;

	private int lastTrackFrame;

	private bool isDragging;

	private bool canDrag;

	public bool forwardMouseEvents = true;

	protected bool isRayOver => lastTrackFrame + 5 > Time.frameCount;

	protected override void Start()
	{
		if (forwardMouseEvents)
		{
			base.Start();
		}
	}

	public void OnRayHit(RaycastHit hit)
	{
		if (lastTrackFrame == 0)
		{
			OnMouseEnter();
		}
		else if (Input.GetMouseButtonDown(0))
		{
			canDrag = true;
			OnMouseDown();
		}
		else if (Input.GetMouseButtonDown(1))
		{
			OnMouseDown();
		}
		else if (Input.GetMouseButtonUp(0))
		{
			canDrag = false;
			OnMouseUp();
		}
		else if (Input.GetMouseButtonUp(1))
		{
			OnMouseUp();
		}
		else if (Input.GetMouseButton(0) && canDrag)
		{
			isDragging = true;
			OnMouseDrag();
		}
		lastTrackFrame = Time.frameCount;
	}

	protected virtual void LateUpdate()
	{
		if (lastTrackFrame <= 0)
		{
			return;
		}
		if (!isDragging && lastTrackFrame + 5 < Time.frameCount)
		{
			OnMouseExit();
			lastTrackFrame = 0;
		}
		if (Input.GetMouseButton(0) && canDrag)
		{
			OnMouseDrag();
		}
		else if (Input.GetMouseButtonUp(0))
		{
			if (!isRayOver && isDragging)
			{
				OnMouseUp();
			}
			canDrag = false;
			isDragging = false;
		}
	}
}
