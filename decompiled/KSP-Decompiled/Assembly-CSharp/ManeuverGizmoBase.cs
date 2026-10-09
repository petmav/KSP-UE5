using System;
using UnityEngine;

public class ManeuverGizmoBase : MonoBehaviour, IMouseEvents
{
	public Transform grabArea;

	public Transform buttonRoot;

	public Transform handlesRoot;

	public ManeuverGizmoHandle handlePrograde;

	public ManeuverGizmoHandle handleRetrograde;

	public ManeuverGizmoHandle handleNormal;

	public ManeuverGizmoHandle handleAntiNormal;

	public ManeuverGizmoHandle handleRadialIn;

	public ManeuverGizmoHandle handleRadialOut;

	public Camera camera;

	public Transform[] cameraFacingBillboards;

	protected Material grabMat;

	protected Color normal;

	protected Color highlight;

	protected Color press;

	protected Color willDelete;

	protected bool hover;

	public Callback OnGizmoDraggedOff = delegate
	{
	};

	public Callback OnMinimize = delegate
	{
	};

	public Callback OnDelete = delegate
	{
	};

	public double sensitivity = 2.0;

	public double multiplier = 5.0;

	public Vector3d DeltaV;

	public double UT;

	protected bool buttonMode;

	public float screenSize = 128f;

	private float lerpedScreenSize;

	protected float rootsScale;

	protected float rootsLerpedScale;

	public static bool HasMouseFocus;

	protected bool mouseOverGizmo;

	public bool MouseOverGizmo => mouseOverGizmo;

	public virtual void SetMouseOverGizmo(bool h)
	{
		if (h)
		{
			mouseOverGizmo = true;
			HasMouseFocus = true;
		}
		else if (!handlePrograde.Hover && !handleRetrograde.Hover && !handleNormal.Hover && !handleAntiNormal.Hover && !handleRadialIn.Hover && !handleRadialOut.Hover)
		{
			mouseOverGizmo = false;
			HasMouseFocus = false;
		}
	}

	public void ChangeToButtonMode()
	{
		buttonMode = true;
	}

	protected virtual void Start()
	{
		grabMat = grabArea.GetComponent<Renderer>().material;
		normal = grabMat.color;
		highlight = grabMat.color * 1.2f;
		press = grabMat.color * 2f;
		rootsScale = 0f;
		rootsLerpedScale = 0f;
		handlePrograde.OnHandleUpdate = OnProgradeUpdate;
		handleRetrograde.OnHandleUpdate = OnRetrogradeUpdate;
		handleNormal.OnHandleUpdate = OnNormalUpdate;
		handleAntiNormal.OnHandleUpdate = OnAntinormalUpdate;
		handleRadialIn.OnHandleUpdate = OnRadialInUpdate;
		handleRadialOut.OnHandleUpdate = OnRadialOutUpdate;
		lerpedScreenSize = 0f;
	}

	protected virtual void LateUpdate()
	{
		lerpedScreenSize = Mathf.Lerp(lerpedScreenSize, screenSize, 8f * Time.deltaTime);
		base.transform.localScale = Vector3.one * (lerpedScreenSize / (float)Screen.height) * (Mathf.Tan(camera.fieldOfView * ((float)Math.PI / 180f)) * (camera.transform.position - base.transform.position).magnitude);
		base.transform.localScale = base.transform.lossyScale;
		int i = 0;
		for (int num = cameraFacingBillboards.Length; i < num; i++)
		{
			cameraFacingBillboards[i].rotation = Quaternion.LookRotation(camera.transform.position - base.transform.position, camera.transform.up);
		}
		if (!buttonMode)
		{
			rootsScale = 0f;
			if (rootsLerpedScale > 0.01f)
			{
				if (!handlesRoot.gameObject.activeInHierarchy)
				{
					handlesRoot.gameObject.SetActive(value: true);
				}
				rootsLerpedScale = Mathf.Lerp(rootsLerpedScale, rootsScale, 8f * Time.deltaTime);
				buttonRoot.localScale = Vector3.one * rootsLerpedScale;
				handlesRoot.localScale = Vector3.one * (1f - rootsLerpedScale);
			}
			else if (buttonRoot.gameObject.activeInHierarchy)
			{
				buttonRoot.gameObject.SetActive(value: false);
			}
		}
		else
		{
			rootsScale = 1f;
			if (rootsLerpedScale < 0.99f)
			{
				if (!buttonRoot.gameObject.activeInHierarchy)
				{
					buttonRoot.gameObject.SetActive(value: true);
				}
				rootsLerpedScale = Mathf.Lerp(rootsLerpedScale, rootsScale, 8f * Time.deltaTime);
				buttonRoot.localScale = Vector3.one * rootsLerpedScale;
				handlesRoot.localScale = Vector3.one * (1f - rootsLerpedScale);
			}
			else if (handlesRoot.gameObject.activeInHierarchy)
			{
				handlesRoot.gameObject.SetActive(value: false);
			}
		}
		if (lerpedScreenSize < 1f)
		{
			OnMinimize();
		}
	}

	public virtual void Terminate()
	{
		hover = false;
		SetMouseOverGizmo(h: false);
		grabMat.color = normal;
		UnityEngine.Object.Destroy(base.gameObject);
	}

	protected virtual void OnProgradeUpdate(float value)
	{
		DeltaV += Vector3d.forward * Math.Pow(Math.Abs(value), sensitivity) * multiplier * Math.Sign(value);
	}

	protected virtual void OnRetrogradeUpdate(float value)
	{
		DeltaV -= Vector3d.forward * Math.Pow(Math.Abs(value), sensitivity) * multiplier * Math.Sign(value);
	}

	protected virtual void OnNormalUpdate(float value)
	{
		DeltaV += Vector3d.up * Math.Pow(Math.Abs(value), sensitivity) * multiplier * Math.Sign(value);
	}

	protected virtual void OnAntinormalUpdate(float value)
	{
		DeltaV -= Vector3d.up * Math.Pow(Math.Abs(value), sensitivity) * multiplier * Math.Sign(value);
	}

	protected virtual void OnRadialInUpdate(float value)
	{
		DeltaV -= Vector3d.right * Math.Pow(Math.Abs(value), sensitivity) * multiplier * Math.Sign(value);
	}

	protected virtual void OnRadialOutUpdate(float value)
	{
		DeltaV += Vector3d.right * Math.Pow(Math.Abs(value), sensitivity) * multiplier * Math.Sign(value);
	}

	public MonoBehaviour GetInstance()
	{
		return this;
	}

	public virtual void OnMouseDown()
	{
		grabMat.color = press;
	}

	public virtual void OnMouseDrag()
	{
	}

	public virtual void OnMouseEnter()
	{
		hover = true;
		SetMouseOverGizmo(h: true);
		grabMat.color = highlight;
	}

	public virtual void OnMouseExit()
	{
		hover = false;
		SetMouseOverGizmo(h: false);
		grabMat.color = normal;
	}

	public virtual void OnMouseUp()
	{
		grabMat.color = (hover ? highlight : normal);
	}
}
