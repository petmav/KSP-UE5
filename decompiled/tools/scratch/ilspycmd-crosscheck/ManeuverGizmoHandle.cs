using UnityEngine;

public class ManeuverGizmoHandle : MonoBehaviour, IMouseEvents
{
	public delegate void HandleUpdate(float value);

	public enum HandleColors
	{
		NORMAL,
		HIGHLIGHT,
		PRESS
	}

	public Transform axis;

	public Transform flag;

	private ManeuverGizmoBase gizmo;

	private Material axisMat;

	private Material flagMat;

	private Color flagNormal;

	private Color flagHighlight;

	private Color flagPress;

	private Color axisNormal;

	private Color axisHighlight;

	private Color axisPress;

	private bool hover;

	private bool drag;

	private Vector3 initialAxisScale;

	private Vector3 initialFlagScale;

	private Vector3 initialPos;

	private Vector2 mousePosAtGrabTime;

	private Vector2 mouseScreenAxis;

	private Vector2 screenAxis;

	private float maxPullScale = 2f;

	private float maxValueLimit = 1f;

	private float minValueLimit = -0.25f;

	private float maxDragPixels;

	public float currentDrag;

	public float currentValue;

	private float scrollWheelMinSens = 0.1f;

	private float scrollWheelSensBoost = 0.6f;

	private float scrollWheelBoostDecay = 5f;

	private float currentScrollWheelBoost;

	public HandleUpdate OnHandleUpdate = delegate
	{
	};

	public bool Hover => hover;

	public bool Drag => drag;

	private void Start()
	{
		gizmo = base.transform.GetComponentInParent<ManeuverGizmoBase>();
		axisMat = axis.GetComponent<Renderer>().material;
		flagMat = flag.GetComponent<Renderer>().material;
		flagNormal = flagMat.color;
		flagHighlight = flagMat.color * 1.2f;
		flagPress = flagMat.color * 2f;
		axisNormal = axisMat.color;
		axisHighlight = axisMat.color * 1.2f;
		axisPress = axisMat.color * 2f;
		initialPos = flag.localPosition;
		initialFlagScale = flag.localScale;
		initialAxisScale = axis.localScale;
		currentScrollWheelBoost = scrollWheelMinSens;
		SetHandleColor(HandleColors.NORMAL);
		flag.localScale = initialFlagScale * 0.8f;
	}

	public void OnMouseEnter()
	{
		hover = true;
		gizmo.SetMouseOverGizmo(h: true);
		flag.localScale = initialFlagScale * 1f;
		SetHandleColor(HandleColors.HIGHLIGHT);
	}

	public void OnMouseDown()
	{
		SetHandleColor(HandleColors.PRESS);
		drag = true;
		mousePosAtGrabTime = Input.mousePosition;
	}

	public void OnMouseDrag()
	{
		screenAxis = ((Vector2)(gizmo.camera.WorldToScreenPoint(flag.position) - gizmo.camera.WorldToScreenPoint(base.transform.position))).normalized;
		maxDragPixels = ((Vector2)(gizmo.camera.WorldToScreenPoint(base.transform.TransformPoint(initialPos)) - gizmo.camera.WorldToScreenPoint(base.transform.position))).magnitude * maxPullScale;
		mouseScreenAxis = (Vector2)Input.mousePosition - mousePosAtGrabTime;
		currentDrag = Vector2.Dot(mouseScreenAxis, screenAxis);
		currentValue = Mathf.Clamp(currentDrag / maxDragPixels, minValueLimit, maxValueLimit);
		UpdateHandle();
	}

	public void OnMouseUp()
	{
		currentValue = 0f;
		flag.localPosition = initialPos;
		axis.localScale = initialAxisScale;
		drag = false;
		if (!hover)
		{
			flag.localScale = initialFlagScale;
		}
		SetHandleColor(hover ? HandleColors.HIGHLIGHT : HandleColors.NORMAL);
	}

	public void OnMouseExit()
	{
		if (!drag)
		{
			SetHandleColor(HandleColors.NORMAL);
			flag.localScale = initialFlagScale * 0.8f;
		}
		hover = false;
		gizmo.SetMouseOverGizmo(h: false);
	}

	public MonoBehaviour GetInstance()
	{
		return this;
	}

	public void SetHandleColor(HandleColors c)
	{
		switch (c)
		{
		case HandleColors.NORMAL:
			flagMat.color = flagNormal;
			axisMat.color = axisNormal;
			break;
		case HandleColors.HIGHLIGHT:
			flagMat.color = flagHighlight;
			axisMat.color = axisHighlight;
			break;
		case HandleColors.PRESS:
			flagMat.color = flagPress;
			axisMat.color = axisPress;
			break;
		}
	}

	private void Update()
	{
		if (drag)
		{
			return;
		}
		if (GameSettings.AXIS_MOUSEWHEEL.GetAxis() != 0f && hover)
		{
			currentValue = Mathf.Clamp(currentValue + GameSettings.AXIS_MOUSEWHEEL.GetAxis() * currentScrollWheelBoost, minValueLimit, maxValueLimit);
			currentScrollWheelBoost += scrollWheelSensBoost;
			OnHandleUpdate(currentValue);
			return;
		}
		currentScrollWheelBoost = Mathf.Max(currentScrollWheelBoost - scrollWheelBoostDecay * Time.deltaTime, scrollWheelMinSens);
		if (currentValue > 0f)
		{
			currentValue = Mathf.Max(currentValue - scrollWheelBoostDecay * Time.deltaTime, 0f);
			OnHandleUpdate(currentValue);
		}
		if (currentValue < 0f)
		{
			currentValue = Mathf.Min(currentValue + scrollWheelBoostDecay * Time.deltaTime, 0f);
			OnHandleUpdate(currentValue);
		}
	}

	private void UpdateHandle()
	{
		flag.localPosition = initialPos + flag.localPosition.normalized * currentValue;
		axis.localScale = initialAxisScale + new Vector3(0f, 0f, currentValue * maxPullScale * initialAxisScale.z);
		OnHandleUpdate(currentValue);
	}
}
