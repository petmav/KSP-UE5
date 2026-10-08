using UnityEngine;
using UnityEngine.UI;

public class EditorVesselOverlays : MonoBehaviour
{
	public static EditorVesselOverlays fetch;

	public Button toggleCoMbtn;

	public Button toggleCoTbtn;

	public Button toggleCoLbtn;

	public EditorMarker_CoM CoMmarker;

	public EditorMarker_CoT CoTmarker;

	public EditorMarker_CoL CoLmarker;

	public float referenceAirSpeed;

	public float referencePitch;

	protected void Awake()
	{
		if ((bool)fetch)
		{
			Object.Destroy(this);
		}
		else
		{
			fetch = this;
		}
	}

	protected void Start()
	{
		toggleCoMbtn.onClick.AddListener(ToggleCoM);
		toggleCoTbtn.onClick.AddListener(ToggleCoT);
		toggleCoLbtn.onClick.AddListener(ToggleCoL);
		EditorFacility editorFacility = EditorDriver.editorFacility;
		if (editorFacility != EditorFacility.VAB && editorFacility == EditorFacility.SPH)
		{
			referencePitch = 1f;
		}
		else
		{
			referencePitch = 0f;
		}
		if ((bool)CoMmarker)
		{
			CoMmarker = Object.Instantiate(CoMmarker);
			CoMmarker.gameObject.SetActive(value: false);
		}
		if ((bool)CoTmarker)
		{
			CoTmarker = Object.Instantiate(CoTmarker);
			CoTmarker.gameObject.SetActive(value: false);
		}
		if ((bool)CoLmarker)
		{
			CoLmarker = Object.Instantiate(CoLmarker);
			CoLmarker.gameObject.SetActive(value: false);
		}
		GameEvents.onEditorPartEvent.Add(OnEditorPartEvent);
		GameEvents.onEditorRestart.Add(OnEditorRestart);
	}

	protected void OnDestroy()
	{
		GameEvents.onEditorPartEvent.Remove(OnEditorPartEvent);
		GameEvents.onEditorRestart.Remove(OnEditorRestart);
		if (fetch != null && fetch == this)
		{
			fetch = null;
		}
	}

	protected void OnEditorPartEvent(ConstructionEventType evt, Part p)
	{
		if (evt == ConstructionEventType.PartDeleted && p == EditorLogic.RootPart)
		{
			DisableMarkers();
		}
	}

	protected void OnEditorRestart()
	{
		DisableMarkers();
	}

	protected void Update()
	{
		if ((bool)CoLmarker)
		{
			CoLmarker.referencePitch = referencePitch;
			CoLmarker.referenceSpeed = referenceAirSpeed;
		}
	}

	protected void DisableMarkers()
	{
		if ((bool)CoMmarker)
		{
			CoMmarker.gameObject.SetActive(value: false);
		}
		if ((bool)CoTmarker)
		{
			CoTmarker.gameObject.SetActive(value: false);
		}
		if ((bool)CoLmarker)
		{
			CoLmarker.gameObject.SetActive(value: false);
		}
	}

	public void ToggleCoM()
	{
		if (EditorLogic.fetch.ship.parts.Count != 0 && (bool)CoMmarker)
		{
			CoMmarker.rootPart = EditorLogic.RootPart;
			CoMmarker.gameObject.SetActive(!CoMmarker.gameObject.activeInHierarchy);
		}
	}

	public void ToggleCoT()
	{
		if (EditorLogic.fetch.ship.parts.Count != 0 && (bool)CoTmarker)
		{
			CoTmarker.rootPart = EditorLogic.RootPart;
			CoTmarker.gameObject.SetActive(!CoTmarker.gameObject.activeInHierarchy);
		}
	}

	public void ToggleCoL()
	{
		if (EditorLogic.fetch.ship.parts.Count != 0 && (bool)CoLmarker)
		{
			CoLmarker.rootPart = EditorLogic.RootPart;
			CoLmarker.gameObject.SetActive(!CoLmarker.gameObject.activeInHierarchy);
		}
	}
}
