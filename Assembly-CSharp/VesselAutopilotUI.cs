using System.Collections.Generic;
using KSP.UI;
using UnityEngine;

[RequireComponent(typeof(Canvas))]
public class VesselAutopilotUI : MonoBehaviour
{
	public UIStateToggleButton[] modeButtons;

	private static int updateWaitFrames = 20;

	private int requiresUpdate = updateWaitFrames;

	private VesselAutopilot.AutopilotMode[] activeModes = new VesselAutopilot.AutopilotMode[0];

	private Canvas canvas;

	private bool uiIsActive = true;

	private void Awake()
	{
		canvas = GetComponent<Canvas>();
	}

	private void Start()
	{
		int i = 0;
		for (int num = modeButtons.Length; i < num; i++)
		{
			int itr = i;
			modeButtons[i].onClick.AddListener(delegate
			{
				OnClickButton(itr);
			});
		}
		SetButtonTrue(0);
		GameEvents.onVesselChange.Add(OnVesselChange);
		GameEvents.onKerbalLevelUp.Add(OnKerbalLevelUp);
		GameEvents.OnGameSettingsApplied.Add(onGameParametersChanged);
	}

	private void OnDestroy()
	{
		GameEvents.onVesselChange.Remove(OnVesselChange);
		GameEvents.onKerbalLevelUp.Remove(OnKerbalLevelUp);
	}

	private void LateUpdate()
	{
		if (requiresUpdate > 0)
		{
			requiresUpdate--;
			if (requiresUpdate == 0)
			{
				SetPilotSkill();
			}
		}
		if (!ToggleUI(FlightGlobals.ActiveVessel.ActionGroups[KSPActionGroup.SAS] && FlightGlobals.ActiveVessel.IsControllable && !FlightGlobals.ActiveVessel.isEVA))
		{
			return;
		}
		if (activeModes.Length == 0)
		{
			if (FlightGlobals.ActiveVessel.Autopilot.Enabled)
			{
				FlightGlobals.ActiveVessel.Autopilot.Disable();
			}
			ToggleUI(enabledState: false);
			return;
		}
		if (activeModes.Length == 1)
		{
			if (!modeButtons[0].gameObject.activeSelf)
			{
				modeButtons[0].gameObject.SetActive(value: true);
			}
			if (!modeButtons[0].StateBool)
			{
				modeButtons[0].SetState(state: true);
			}
			for (int i = 1; i < modeButtons.Length; i++)
			{
				if (modeButtons[i].gameObject.activeSelf)
				{
					modeButtons[i].gameObject.SetActive(value: false);
				}
			}
			if (FlightGlobals.ActiveVessel.Autopilot.Mode != 0)
			{
				FlightGlobals.ActiveVessel.Autopilot.SetMode(VesselAutopilot.AutopilotMode.StabilityAssist);
			}
			return;
		}
		int j = 0;
		for (int num = modeButtons.Length; j < num; j++)
		{
			VesselAutopilot.AutopilotMode autopilotMode = (VesselAutopilot.AutopilotMode)j;
			if (IsSkillActive(autopilotMode))
			{
				if (!modeButtons[j].gameObject.activeSelf)
				{
					modeButtons[j].gameObject.SetActive(value: true);
				}
				if (FlightGlobals.ActiveVessel.Autopilot.CanSetMode(autopilotMode))
				{
					if (!modeButtons[j].interactable)
					{
						modeButtons[j].interactable = true;
					}
					if (FlightGlobals.ActiveVessel.Autopilot.Mode == autopilotMode)
					{
						if (!modeButtons[j].StateBool)
						{
							modeButtons[j].SetState(state: true);
						}
					}
					else if (modeButtons[j].StateBool)
					{
						modeButtons[j].SetState(state: false);
					}
				}
				else if (modeButtons[j].interactable)
				{
					modeButtons[j].interactable = false;
				}
			}
			else if (modeButtons[j].gameObject.activeSelf)
			{
				modeButtons[j].gameObject.SetActive(value: false);
			}
		}
	}

	private bool ToggleUI(bool enabledState)
	{
		uiIsActive = enabledState;
		if (canvas.enabled != uiIsActive)
		{
			canvas.enabled = uiIsActive;
		}
		return uiIsActive;
	}

	private void OnVesselChange(Vessel v)
	{
		requiresUpdate = updateWaitFrames;
		OnClickButton(0);
	}

	private void OnKerbalLevelUp(ProtoCrewMember pcm)
	{
		requiresUpdate = 2;
	}

	private void onGameParametersChanged()
	{
		requiresUpdate = 2;
	}

	private void OnClickButton(int buttonIndex)
	{
		int mode = (int)FlightGlobals.ActiveVessel.Autopilot.Mode;
		if (buttonIndex == mode)
		{
			SetButtonTrue(buttonIndex);
		}
		else if (FlightGlobals.ActiveVessel.Autopilot.SetMode((VesselAutopilot.AutopilotMode)buttonIndex))
		{
			SetButtonTrue(buttonIndex);
		}
	}

	private void SetButtonTrue(int btn)
	{
		int i = 0;
		for (int num = modeButtons.Length; i < num; i++)
		{
			if (i == btn)
			{
				if (modeButtons[i].gameObject.activeSelf && modeButtons[i].interactable && !modeButtons[i].StateBool)
				{
					modeButtons[i].SetState(state: true);
				}
			}
			else if (modeButtons[i].gameObject.activeSelf && modeButtons[i].interactable && modeButtons[i].StateBool)
			{
				modeButtons[i].SetState(state: false);
			}
		}
	}

	private int GetButtonIndex(UIStateToggleButton btn)
	{
		int num = 0;
		int num2 = modeButtons.Length;
		while (true)
		{
			if (num < num2)
			{
				if (modeButtons[num] == btn)
				{
					break;
				}
				num++;
				continue;
			}
			return -1;
		}
		return num;
	}

	private void SetPilotSkill()
	{
		List<VesselAutopilot.AutopilotMode> list = new List<VesselAutopilot.AutopilotMode>();
		int i = 0;
		for (int num = modeButtons.Length; i < num; i++)
		{
			VesselAutopilot.AutopilotMode autopilotMode = (VesselAutopilot.AutopilotMode)i;
			if (autopilotMode.AvailableAtLevel(FlightGlobals.ActiveVessel))
			{
				list.Add(autopilotMode);
			}
		}
		activeModes = list.ToArray();
	}

	private bool IsSkillActive(VesselAutopilot.AutopilotMode mode)
	{
		int num = 0;
		int num2 = activeModes.Length;
		while (true)
		{
			if (num < num2)
			{
				if (mode == activeModes[num])
				{
					break;
				}
				num++;
				continue;
			}
			return false;
		}
		return true;
	}
}
