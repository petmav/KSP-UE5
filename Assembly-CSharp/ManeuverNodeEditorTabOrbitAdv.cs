using TMPro;
using UnityEngine;

public class ManeuverNodeEditorTabOrbitAdv : ManeuverNodeEditorTab
{
	[SerializeField]
	private TextMeshProUGUI orbitArgumentOfPeriapsis;

	[SerializeField]
	private TextMeshProUGUI orbitLongitudeOfAscendingNode;

	[SerializeField]
	private TextMeshProUGUI ejectionAngle;

	[SerializeField]
	private TextMeshProUGUI orbitEccentricity;

	[SerializeField]
	private TextMeshProUGUI orbitInclination;

	private Orbit orbitToDisplay;

	private bool patchesAheadLimitOK;

	private void Start()
	{
		if ((bool)FlightUIModeController.Instance)
		{
			mannodeEditorManager = FlightUIModeController.Instance.manNodeHandleEditor.GetComponent<ManeuverNodeEditorManager>();
		}
		UpdateUIElements();
	}

	public override void SetInitialValues()
	{
		patchesAheadLimitOK = GameVariables.Instance.GetPatchesAheadLimit(ScenarioUpgradeableFacilities.GetFacilityLevel(SpaceCenterFacility.TrackingStation)) > 0;
		UpdateUIElements();
	}

	public override void UpdateUIElements()
	{
		if ((bool)FlightUIModeController.Instance)
		{
			mannodeEditorManager = FlightUIModeController.Instance.manNodeHandleEditor.GetComponent<ManeuverNodeEditorManager>();
		}
		if (mannodeEditorManager.SelectedManeuverNode == null)
		{
			orbitToDisplay = FlightGlobals.ActiveVessel.orbit;
		}
		else
		{
			orbitToDisplay = mannodeEditorManager.SelectedManeuverNode.nextPatch;
		}
		orbitInclination.text = orbitToDisplay.inclination.ToString("F1") + " °";
		orbitEccentricity.text = orbitToDisplay.eccentricity.ToString("F4");
		ejectionAngle.text = OrbitUtil.CurrentEjectionAngle(orbitToDisplay, Planetarium.GetUniversalTime()).ToString("F1") + " °";
		orbitArgumentOfPeriapsis.text = orbitToDisplay.argumentOfPeriapsis.ToString("F1") + " °";
		orbitLongitudeOfAscendingNode.text = orbitToDisplay.LAN.ToString("F1") + " °";
	}

	public override bool IsTabInteractable()
	{
		if (patchesAheadLimitOK)
		{
			return InputLockManager.IsUnlocked(ControlTypes.FLIGHTUIMODE);
		}
		return false;
	}
}
