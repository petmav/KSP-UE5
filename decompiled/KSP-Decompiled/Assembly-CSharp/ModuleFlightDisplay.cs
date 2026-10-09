using KSP.Localization;
using UnityEngine;

public class ModuleFlightDisplay : PartModule
{
	private static Rect _windowPosition = new Rect(10f, 60f, 285f, 450f);

	private GUIStyle _windowStyle;

	private GUIStyle _labelStyle;

	private GUIStyle _centerStyle;

	private bool _hasInitStyles;

	private bool _guiIsActive;

	public bool GuiIsActive
	{
		get
		{
			return _guiIsActive;
		}
		set
		{
			_guiIsActive = value;
		}
	}

	[KSPEvent(unfocusedRange = 3f, active = true, guiActiveUnfocused = true, externalToEVAOnly = true, guiActive = true, guiActiveEditor = false, guiName = "#autoLOC_6001811")]
	public void ToggleGui()
	{
		ModuleFlightDisplay activeGUI = GetActiveGUI();
		if (activeGUI == null)
		{
			_guiIsActive = true;
		}
		else
		{
			activeGUI.TurnOffGui();
		}
	}

	private void TurnOffGui()
	{
		_guiIsActive = false;
	}

	private ModuleFlightDisplay GetActiveGUI()
	{
		int count = FlightGlobals.Vessels.Count;
		int num = 0;
		ModuleFlightDisplay moduleFlightDisplay;
		while (true)
		{
			if (num < count)
			{
				Vessel vessel = FlightGlobals.Vessels[num];
				if (!vessel.packed)
				{
					moduleFlightDisplay = vessel.FindPartModuleImplementing<ModuleFlightDisplay>();
					if (moduleFlightDisplay != null && moduleFlightDisplay.GuiIsActive)
					{
						break;
					}
				}
				num++;
				continue;
			}
			return null;
		}
		return moduleFlightDisplay;
	}

	public override void OnStart(StartState state)
	{
		if (state != StartState.Editor)
		{
			if (!_hasInitStyles)
			{
				InitStyles();
			}
			Setup();
		}
	}

	private void Setup()
	{
	}

	private void OnDraw()
	{
		if (base.vessel == FlightGlobals.ActiveVessel)
		{
			_windowPosition = GUILayout.Window(10, _windowPosition, OnWindow, "Flight Debugger", _windowStyle);
		}
	}

	private void OnWindow(int windowId)
	{
		GUILayout.BeginVertical();
		if (!base.vessel.IsControllable)
		{
			_guiIsActive = false;
		}
		GUILayout.BeginHorizontal();
		GUILayout.Label("State:", _labelStyle, GUILayout.Width(100f));
		GUILayout.Label(GetControlState(), _labelStyle, GUILayout.Width(160f));
		GUILayout.EndHorizontal();
		GUILayout.BeginHorizontal();
		GUILayout.Label("Signal:", _labelStyle, GUILayout.Width(100f));
		GUILayout.Label(GetSignalStrength(), _labelStyle, GUILayout.Width(160f));
		GUILayout.EndHorizontal();
		GUILayout.BeginHorizontal();
		GUILayout.Label("First Hop:", _labelStyle, GUILayout.Width(100f));
		GUILayout.Label(GetFirstHop(), _labelStyle, GUILayout.Width(160f));
		GUILayout.EndHorizontal();
		GUILayout.BeginHorizontal();
		GUILayout.Label("Total Hops:", _labelStyle, GUILayout.Width(100f));
		GUILayout.Label(GetTotalHops(), _labelStyle, GUILayout.Width(160f));
		GUILayout.EndHorizontal();
		GUILayout.BeginHorizontal();
		GUILayout.Label("Control Point:", _labelStyle, GUILayout.Width(100f));
		GUILayout.Label(GetLastHop(), _labelStyle, GUILayout.Width(160f));
		GUILayout.EndHorizontal();
		GUILayout.EndVertical();
		GUI.DragWindow();
	}

	private string GetSignalStrength()
	{
		return "<color=green>" + 100 + "%</color>";
	}

	private string GetFirstHop()
	{
		string text = "none";
		string text2 = "orange";
		if (base.vessel.connection.ControlPath.Count > 0)
		{
			text = Localizer.Format(base.vessel.connection.ControlPath[0].end.displayName);
			text2 = "green";
		}
		return "<color=" + text2 + ">" + text + "</color>";
	}

	private string GetTotalHops()
	{
		string text = "0";
		string text2 = "orange";
		if (base.vessel.connection.ControlPath.Count > 0)
		{
			text = base.vessel.connection.ControlPath.Count.ToString();
			text2 = "green";
		}
		return "<color=" + text2 + ">" + text + "</color>";
	}

	private string GetLastHop()
	{
		string text = "none";
		string text2 = "orange";
		if (base.vessel.connection.ControlPath.Count > 0)
		{
			text = base.vessel.connection.ControlPath[base.vessel.connection.ControlPath.Count - 1].ToString();
			text2 = "green";
		}
		return "<color=" + text2 + ">" + text + "</color>";
	}

	private string GetControlState()
	{
		ModuleCommand moduleCommand = base.part.FindModuleImplementing<ModuleCommand>();
		string text = "green";
		switch (moduleCommand.ModuleState)
		{
		case ModuleCommand.ModuleControlState.NotEnoughCrew:
			text = "red";
			break;
		case ModuleCommand.ModuleControlState.NotEnoughResources:
			text = "red";
			break;
		case ModuleCommand.ModuleControlState.PartialManned:
			text = "red";
			break;
		case ModuleCommand.ModuleControlState.NoControlPoint:
			text = "red";
			break;
		case ModuleCommand.ModuleControlState.TouristCrew:
			text = "red";
			break;
		case ModuleCommand.ModuleControlState.PartialProbe:
			text = "yellow";
			break;
		}
		return string.Concat("<color=", text, ">", moduleCommand.ModuleState, "</color>");
	}

	private void InitStyles()
	{
		_windowStyle = new GUIStyle(HighLogic.Skin.window);
		_windowStyle.fixedWidth = 285f;
		_windowStyle.fixedHeight = 160f;
		_labelStyle = new GUIStyle(HighLogic.Skin.label);
		_labelStyle.stretchHeight = true;
		_centerStyle = new GUIStyle();
		_centerStyle.padding = new RectOffset(5, 5, 0, 0);
		_hasInitStyles = true;
	}
}
