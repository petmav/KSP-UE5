using UnityEngine;

public class AdvancedScreen : MonoBehaviour
{
	public UIStateToggleBtn AMDMode;

	public UIStateToggleBtn noLog;

	public SpriteText noLogText;

	public UIStateToggleBtn popUp;

	public SpriteText popUpText;

	public UIStateToggleBtn extCharMode;

	public SpriteText exCharText;

	public UIStateToggleBtn showHideLauncher;

	public AdvancedSettings advSettings;

	public UIStateToggleBtn x64But;

	public SpriteText x64Text;

	private void Start()
	{
		advSettings = GameObject.Find("AdvSettings").GetComponent<AdvancedSettings>();
		AMDMode.AddValueChangedDelegate(advChangedDel);
		noLog.AddValueChangedDelegate(advChangedDel);
		popUp.AddValueChangedDelegate(advChangedDel);
		extCharMode.AddValueChangedDelegate(advChangedDel);
		showHideLauncher.AddValueChangedDelegate(advChangedDel);
		x64But.AddValueChangedDelegate(advChangedDel);
		setInitialValues();
		if (Application.platform == RuntimePlatform.OSXPlayer)
		{
			noLog.controlIsEnabled = false;
			noLogText.Text = "This option is unavailable for this platform";
		}
		if (Application.platform != RuntimePlatform.WindowsPlayer)
		{
			popUp.controlIsEnabled = false;
			popUpText.Text = "This option is unavailable for this platform";
			x64But.controlIsEnabled = false;
			x64Text.Text = "This option is unavailable for this platform";
		}
		if (Application.platform != RuntimePlatform.LinuxPlayer)
		{
			extCharMode.controlIsEnabled = false;
			exCharText.Text = "This option is unavailable for this platform";
		}
	}

	private void setInitialValues()
	{
		if (advSettings.AMD)
		{
			AMDMode.SetToggleState("Checked");
		}
		if (advSettings.fakeFullS)
		{
			popUp.SetToggleState("Checked");
		}
		if (advSettings.noLog)
		{
			noLog.SetToggleState("Checked");
		}
		if (advSettings.extChar)
		{
			extCharMode.SetToggleState("Checked");
		}
		if (advSettings.dontShowLauncher)
		{
			showHideLauncher.SetToggleState("Checked");
		}
		if (advSettings.x64Player)
		{
			x64But.SetToggleState("Checked");
		}
	}

	public void commitCurrentSetting()
	{
		if (AMDMode.StateName == "Checked")
		{
			advSettings.setAMDState("True");
		}
		else
		{
			advSettings.setAMDState("False");
		}
		if (noLog.StateName == "Checked")
		{
			advSettings.setunLogState("True");
		}
		else
		{
			advSettings.setunLogState("False");
		}
		if (popUp.StateName == "Checked")
		{
			advSettings.setFullSState("True");
		}
		else
		{
			advSettings.setFullSState("False");
		}
		if (extCharMode.StateName == "Checked")
		{
			advSettings.setCharState("True");
		}
		else
		{
			advSettings.setCharState("False");
		}
		if (showHideLauncher.StateName == "Checked")
		{
			advSettings.setLauncherState("True");
		}
		else
		{
			advSettings.setLauncherState("False");
		}
		if (x64But.StateName == "Checked")
		{
			advSettings.setx64tate("True");
		}
		else
		{
			advSettings.setx64tate("False");
		}
	}

	private void advChangedDel(IUIObject obj)
	{
		if (obj == AMDMode)
		{
			if (AMDMode.StateName == "Checked")
			{
				advSettings.setAMDState("True");
			}
			else
			{
				advSettings.setAMDState("False");
			}
		}
		if (obj == noLog && noLog.controlIsEnabled)
		{
			if (noLog.StateName == "Checked")
			{
				advSettings.setunLogState("True");
			}
			else
			{
				advSettings.setunLogState("False");
			}
		}
		if (obj == popUp)
		{
			if (popUp.StateName == "Checked")
			{
				advSettings.setFullSState("True");
			}
			else
			{
				advSettings.setFullSState("False");
			}
		}
		if (obj == extCharMode)
		{
			if (extCharMode.StateName == "Checked")
			{
				advSettings.setCharState("True");
			}
			else
			{
				advSettings.setCharState("False");
			}
		}
		if (obj == showHideLauncher)
		{
			if (showHideLauncher.StateName == "Checked")
			{
				advSettings.setLauncherState("True");
			}
			else
			{
				advSettings.setLauncherState("False");
			}
		}
		if (obj == x64But)
		{
			if (x64But.StateName == "Checked")
			{
				advSettings.setx64tate("True");
			}
			else
			{
				advSettings.setx64tate("False");
			}
		}
	}
}
