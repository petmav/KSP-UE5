using KSP.Localization;
using KSP.UI;
using UnityEngine;
using UnityEngine.UI;

public class ManeuverGizmo : ManeuverGizmoBase
{
	public delegate void HandlesUpdatedCallback(Vector3d dV, double UT);

	public Canvas canvas;

	public Button deleteBtn;

	public Button plusOrbitBtn;

	public Button minusOrbitbtn;

	private PointerEnterExitHandler plusBtnHover;

	private PointerEnterExitHandler minusBtnHover;

	public int orbitsAdded;

	public HandlesUpdatedCallback OnGizmoUpdated = delegate
	{
	};

	public Orbit patchBefore;

	public Orbit patchAhead;

	public PatchedConicRenderer renderer;

	private Vector3 mouseOffset;

	private PatchedConics.PatchCastHit scHit;

	private bool goodHover;

	private bool startDrag;

	private bool justTweaked;

	private double prevUT = -1.0;

	private double prevprevUT = -1.0;

	private bool wasLocked;

	private Vector3 lastMousePosition;

	public override void SetMouseOverGizmo(bool h)
	{
		if (h)
		{
			mouseOverGizmo = true;
			ManeuverGizmoBase.HasMouseFocus = true;
			if ((bool)renderer)
			{
				renderer.SetMouseOverGizmo(h: true);
			}
		}
		else if (!handlePrograde.Hover && !handleRetrograde.Hover && !handleNormal.Hover && !handleAntiNormal.Hover && !handleRadialIn.Hover && !handleRadialOut.Hover && !minusBtnHover.IsOver && !plusBtnHover.IsOver && !hover)
		{
			mouseOverGizmo = false;
			ManeuverGizmoBase.HasMouseFocus = false;
			if ((bool)renderer)
			{
				renderer.SetMouseOverGizmo(h: false);
			}
		}
	}

	protected override void Start()
	{
		base.Start();
		camera = PlanetariumCamera.Camera ?? Camera.main;
		canvas.worldCamera = camera;
		willDelete = XKCDColors.BurntSienna;
		willDelete.a = 0.5f;
		deleteBtn.onClick.AddListener(OnDeletePress);
		plusOrbitBtn.onClick.AddListener(OnPlusOrbitPress);
		minusOrbitbtn.onClick.AddListener(OnMinusOrbitPress);
		plusBtnHover = plusOrbitBtn.GetComponent<PointerEnterExitHandler>();
		minusBtnHover = minusOrbitbtn.GetComponent<PointerEnterExitHandler>();
		orbitsAdded = 0;
		if (patchBefore.eccentricity < 1.0)
		{
			orbitsAdded = (int)((UT - patchBefore.StartUT) / patchBefore.period);
		}
		justTweaked = true;
	}

	public void Setup(ManeuverNode node, PatchedConicRenderer rnd)
	{
		patchBefore = node.patch;
		patchAhead = node.nextPatch;
		renderer = rnd;
		UpdateOrbitButtons();
	}

	public void SetPatches(Orbit patch, Orbit nextPatch, bool reset)
	{
		patchBefore = patch;
		patchAhead = nextPatch;
		if (reset)
		{
			prevUT = -1.0;
			prevprevUT = -1.0;
		}
	}

	protected override void LateUpdate()
	{
		base.LateUpdate();
		if (InputLockManager.IsLocked(ControlTypes.MANNODE_ADDEDIT) && base.MouseOverGizmo && !ManeuverNodeEditorManager.Instance.MouseWithinTool && !ManeuverNodeEditorManager.Instance.JustSelectedNode)
		{
			buttonMode = true;
			if (!wasLocked)
			{
				UpdateOrbitButtons();
			}
			wasLocked = true;
		}
		else
		{
			if (wasLocked)
			{
				UpdateOrbitButtons();
			}
			wasLocked = false;
		}
		if (Input.GetMouseButtonUp(0) && !mouseOverGizmo)
		{
			if (!justTweaked)
			{
				screenSize = 0f;
			}
			else
			{
				justTweaked = false;
			}
		}
		if (Input.GetMouseButtonDown(0) && !mouseOverGizmo)
		{
			justTweaked = false;
		}
		if (UT > Planetarium.GetUniversalTime())
		{
			if (Input.GetMouseButtonDown(1) && mouseOverGizmo && InputLockManager.IsUnlocked(ControlTypes.MANNODE_ADDEDIT) && !ManeuverNodeEditorManager.Instance.MouseWithinTool)
			{
				buttonMode = !buttonMode;
			}
		}
		else
		{
			buttonMode = true;
		}
	}

	private void OnDestroy()
	{
		ManeuverGizmoBase.HasMouseFocus = false;
		if ((bool)renderer)
		{
			renderer.SetMouseOverGizmo(h: false);
		}
	}

	public override void OnMouseDown()
	{
		base.OnMouseDown();
		mouseOffset = camera.WorldToScreenPoint(base.transform.position) - Input.mousePosition;
		mouseOffset = new Vector3(mouseOffset.x, mouseOffset.y, 0f);
		goodHover = true;
		startDrag = true;
	}

	public override void OnMouseDrag()
	{
		base.OnMouseDrag();
		if (!InputLockManager.IsUnlocked(ControlTypes.MANNODE_ADDEDIT) || renderer == null)
		{
			return;
		}
		bool flag = UT < Planetarium.GetUniversalTime();
		if (startDrag)
		{
			lastMousePosition = Input.mousePosition;
			startDrag = false;
		}
		if (lastMousePosition == Input.mousePosition)
		{
			return;
		}
		PatchRendering patchRendering = renderer.FindRenderingForPatch(patchBefore);
		if (patchRendering == null)
		{
			goodHover = false;
		}
		else
		{
			goodHover = PatchedConics.ScreenCastWorker(Input.mousePosition + mouseOffset, patchRendering, out scHit, 100f);
		}
		if (goodHover)
		{
			double uT = UT;
			UT = scHit.UTatTA;
			if (patchBefore.eccentricity < 1.0)
			{
				UT += (double)orbitsAdded * patchBefore.period;
			}
			if (flag && !DraggingForwardInTime())
			{
				UT = uT;
				prevprevUT = prevUT;
				prevUT = UT;
				return;
			}
			prevprevUT = prevUT;
			prevUT = UT;
			if (UT <= patchBefore.StartUT)
			{
				UT -= 1.0;
			}
			if (!double.IsNaN(UT))
			{
				onGizmoUpdate(DeltaV, UT);
			}
		}
		else
		{
			grabMat.color = willDelete;
		}
	}

	private bool DraggingForwardInTime()
	{
		if (prevUT >= 0.0 && prevprevUT >= 0.0 && UT > prevUT)
		{
			return prevUT >= prevprevUT;
		}
		return false;
	}

	public override void OnMouseUp()
	{
		base.OnMouseUp();
		if (!goodHover)
		{
			OnGizmoDraggedOff();
		}
	}

	public void NextOrbit()
	{
		OnPlusOrbitPress();
	}

	public void PreviousOrbit()
	{
		OnMinusOrbitPress();
	}

	protected override void OnProgradeUpdate(float value)
	{
		base.OnProgradeUpdate(value);
		onGizmoUpdate(DeltaV, UT);
	}

	protected override void OnRetrogradeUpdate(float value)
	{
		base.OnRetrogradeUpdate(value);
		onGizmoUpdate(DeltaV, UT);
	}

	protected override void OnNormalUpdate(float value)
	{
		base.OnNormalUpdate(value);
		onGizmoUpdate(DeltaV, UT);
	}

	protected override void OnAntinormalUpdate(float value)
	{
		base.OnAntinormalUpdate(value);
		onGizmoUpdate(DeltaV, UT);
	}

	protected override void OnRadialInUpdate(float value)
	{
		base.OnRadialInUpdate(value);
		onGizmoUpdate(DeltaV, UT);
	}

	protected override void OnRadialOutUpdate(float value)
	{
		base.OnRadialOutUpdate(value);
		onGizmoUpdate(DeltaV, UT);
	}

	private void OnPlusOrbitPress()
	{
		if (patchBefore.eccentricity < 1.0)
		{
			orbitsAdded++;
			UT += patchBefore.period;
			onGizmoUpdate(DeltaV, UT);
		}
		Mouse.Left.ClearMouseState();
	}

	private void OnMinusOrbitPress()
	{
		if (patchBefore.eccentricity < 1.0)
		{
			orbitsAdded--;
			UT -= patchBefore.period;
			onGizmoUpdate(DeltaV, UT);
		}
		Mouse.Left.ClearMouseState();
	}

	private void OnDeletePress()
	{
		if (InputLockManager.IsUnlocked(ControlTypes.MANNODE_DELETE))
		{
			OnDelete();
		}
		else
		{
			ScreenMessages.PostScreenMessage(Localizer.Format("#autoLOC_193630"), 3f, ScreenMessageStyle.UPPER_CENTER);
		}
		Mouse.Left.ClearMouseState();
	}

	private void onGizmoUpdate(Vector3d dV, double UT)
	{
		justTweaked = true;
		OnGizmoUpdated(dV, UT);
		UpdateOrbitButtons();
	}

	public bool PreviousOrbitPossible()
	{
		bool result = false;
		if (InputLockManager.IsUnlocked(ControlTypes.MANNODE_ADDEDIT) && (patchBefore.patchEndTransition == Orbit.PatchTransitionType.MANEUVER || patchBefore.patchEndTransition == Orbit.PatchTransitionType.FINAL || patchBefore.eccentricity <= 1.0) && UT - patchBefore.period >= patchBefore.StartUT)
		{
			result = true;
		}
		return result;
	}

	public bool NextOrbitPossible()
	{
		bool result = false;
		if (InputLockManager.IsUnlocked(ControlTypes.MANNODE_ADDEDIT) && (patchBefore.patchEndTransition == Orbit.PatchTransitionType.MANEUVER || patchBefore.patchEndTransition == Orbit.PatchTransitionType.FINAL || patchBefore.eccentricity <= 1.0))
		{
			result = true;
		}
		return result;
	}

	private void UpdateOrbitButtons()
	{
		minusOrbitbtn.interactable = PreviousOrbitPossible();
		plusOrbitBtn.interactable = NextOrbitPossible();
	}
}
