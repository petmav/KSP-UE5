using UnityEngine;

public class UIScrollKnob : UIButton
{
	protected Vector3 origPos;

	protected UISlider slider;

	protected float maxScrollPos;

	protected Plane ctrlPlane;

	protected Vector2 colliderSizeFactor;

	protected float colliderExtent;

	private float dist;

	private Vector3 inputPoint;

	private Vector3 newPos;

	private Vector3 prevPoint;

	private float discreteStepCount;

	public void SetDiscreteStepCount(float discreteStepCount)
	{
		this.discreteStepCount = discreteStepCount;
	}

	protected override void Awake()
	{
		base.Awake();
		origPos = base.transform.localPosition;
	}

	public override void OnInput(ref POINTER_INFO ptr)
	{
		base.OnInput(ref ptr);
		if (!m_controlIsEnabled)
		{
			return;
		}
		switch (ptr.evt)
		{
		case POINTER_INFO.INPUT_EVENT.PRESS:
			prevPoint = GetLocalInputPoint(ptr.ray);
			if (discreteStepCount > 0f)
			{
				DiscretizePoint(ref prevPoint);
				prevPoint.x = Mathf.Clamp(prevPoint.x, origPos.x, origPos.x + maxScrollPos);
			}
			break;
		case POINTER_INFO.INPUT_EVENT.DRAG:
			inputPoint = GetLocalInputPoint(ptr.ray);
			if (discreteStepCount > 0f)
			{
				DiscretizePoint(ref inputPoint);
			}
			if (discreteStepCount > 0f)
			{
				prevPoint.x = Mathf.Clamp(prevPoint.x, origPos.x, origPos.x + maxScrollPos);
			}
			else
			{
				prevPoint.x = Mathf.Clamp(prevPoint.x, origPos.x - colliderExtent, origPos.x + colliderExtent + maxScrollPos);
			}
			dist = inputPoint.x - prevPoint.x;
			prevPoint = inputPoint;
			newPos = base.transform.localPosition;
			newPos.x = Mathf.Clamp(newPos.x + dist, origPos.x, origPos.x + maxScrollPos);
			base.transform.localPosition = newPos;
			prevPoint.x = Mathf.Clamp(prevPoint.x, origPos.x - colliderExtent, origPos.x + colliderExtent + maxScrollPos);
			slider.ScrollKnobMoved(this, GetScrollPos());
			break;
		}
	}

	public void SetStartPos(Vector3 startPos)
	{
		origPos = startPos;
	}

	protected Vector3 GetLocalInputPoint(Ray ray)
	{
		ctrlPlane.SetNormalAndPosition(base.transform.forward * -1f, base.transform.position);
		ctrlPlane.Raycast(ray, out dist);
		return base.transform.parent.InverseTransformPoint(ray.origin + ray.direction * dist);
	}

	public override void Copy(SpriteRoot s, ControlCopyFlags flags)
	{
		base.Copy(s, flags);
		if (s is UIScrollKnob)
		{
			UIScrollKnob uIScrollKnob = (UIScrollKnob)s;
			if ((flags & ControlCopyFlags.State) == ControlCopyFlags.State)
			{
				origPos = uIScrollKnob.origPos;
				ctrlPlane = uIScrollKnob.ctrlPlane;
				slider = uIScrollKnob.slider;
			}
			if ((flags & ControlCopyFlags.Settings) == ControlCopyFlags.Settings)
			{
				maxScrollPos = uIScrollKnob.maxScrollPos;
				colliderSizeFactor = uIScrollKnob.colliderSizeFactor;
				discreteStepCount = uIScrollKnob.discreteStepCount;
			}
		}
	}

	public void SetColliderSizeFactor(Vector2 csf)
	{
		colliderSizeFactor = csf;
	}

	public override void UpdateCollider()
	{
		base.UpdateCollider();
		if (GetComponent<Collider>() is BoxCollider && !IsHidden())
		{
			BoxCollider boxCollider = (BoxCollider)GetComponent<Collider>();
			boxCollider.size = new Vector3(boxCollider.size.x * colliderSizeFactor.x, boxCollider.size.y * colliderSizeFactor.y, 0.001f);
			colliderExtent = boxCollider.size.x * 0.5f;
		}
	}

	public float GetScrollPos()
	{
		return (base.transform.localPosition.x - origPos.x) / maxScrollPos;
	}

	public void SetPosition(float pos)
	{
		base.transform.localPosition = origPos + Vector3.right * maxScrollPos * pos;
	}

	public void SetSlider(UISlider s)
	{
		slider = s;
	}

	public UISlider GetSlider()
	{
		return slider;
	}

	public void SetMaxScroll(float max)
	{
		maxScrollPos = max;
	}

	public void SetupAppearance()
	{
		Start();
		InitUVs();
		UpdateUVs();
	}

	public new static UIScrollKnob Create(string name, Vector3 pos)
	{
		GameObject obj = new GameObject(name);
		obj.transform.position = pos;
		return (UIScrollKnob)obj.AddComponent(typeof(UIScrollKnob));
	}

	public new static UIScrollKnob Create(string name, Vector3 pos, Quaternion rotation)
	{
		GameObject obj = new GameObject(name);
		obj.transform.position = pos;
		obj.transform.rotation = rotation;
		return (UIScrollKnob)obj.AddComponent(typeof(UIScrollKnob));
	}

	private void DiscretizePoint(ref Vector3 pt)
	{
		float num = Mathf.Round((pt.x - origPos.x) * discreteStepCount / maxScrollPos);
		pt.x = origPos.x + num * maxScrollPos / discreteStepCount;
	}
}
