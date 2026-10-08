using System;
using KSP.Localization;
using KSP.UI;
using TMPro;
using UnityEngine;
using UnityEngine.UI;

public class AlarmClockUIAlarmRow : MonoBehaviour
{
	private AlarmClockUIFrame appFrame;

	[SerializeField]
	public Button deleteButton;

	[SerializeField]
	public Toggle toggle;

	public Image alarmType;

	public TextMeshProUGUI nameLabel;

	public TextMeshProUGUI timeLabel;

	public Color futureAlarmTextColor = Color.white;

	public Color pastAlarmTextColor = new Color32(103, 113, 125, byte.MaxValue);

	private Color currentTextColor;

	private AlarmTypeBase alarm;

	private Action<AlarmTypeBase, bool> onToggleChanged;

	private double currentUT;

	private double lastUIUpdateUT;

	internal uint AlarmId
	{
		get
		{
			if (alarm != null)
			{
				return alarm.Id;
			}
			return 0u;
		}
	}

	public void Setup(AlarmClockUIFrame appFrame, AlarmTypeBase alarm, bool selected, Transform parent, ToggleGroup toggleGroup, Action<AlarmTypeBase, bool> onToggleChanged)
	{
		this.appFrame = appFrame;
		this.alarm = alarm;
		base.gameObject.transform.SetParent(parent);
		base.gameObject.transform.localScale = Vector3.one;
		nameLabel.text = alarm.title;
		alarmType.sprite = AlarmClockScenario.AlarmSprites[alarm.iconURL];
		toggle.group = toggleGroup;
		toggle.isOn = selected;
		this.onToggleChanged = onToggleChanged;
		toggle.onValueChanged.AddListener(OnToggleChanged);
		UpdateRowUI();
		SetTextColor();
	}

	private void OnToggleChanged(bool value)
	{
		onToggleChanged(alarm, value);
	}

	private void Start()
	{
		deleteButton.onClick.AddListener(DeleteAlarm);
	}

	private void Update()
	{
		UpdateRowUI();
		SetTextColor();
	}

	private void UpdateRowUI()
	{
		currentUT = Planetarium.GetUniversalTime();
		if (currentUT - lastUIUpdateUT > AlarmClockScenario.UIUpdatePeriod)
		{
			UIUpdate();
			lastUIUpdateUT = currentUT;
		}
	}

	private void SetTextColor()
	{
		currentTextColor = ((currentUT > alarm.ut) ? pastAlarmTextColor : futureAlarmTextColor);
		nameLabel.color = currentTextColor;
		timeLabel.color = currentTextColor;
	}

	private void UIUpdate()
	{
		timeLabel.text = appFrame.PrintTimeStampCompact(alarm.TimeToAlarm, days: true, years: true);
	}

	private void OnDestroy()
	{
		deleteButton.onClick.AddListener(DeleteAlarm);
	}

	private void DeleteAlarm()
	{
		if (alarm != null)
		{
			if (GameSettings.SHOW_DELETE_ALARM_CONFIRMATION)
			{
				UIConfirmDialog.Spawn(Localizer.Format("#autoLOC_8003552"), Localizer.Format("#autoLOC_8003553", alarm.title, AlarmClockUIFrame.PrintDate(alarm.ut, includeTime: true, includeSeconds: true, timeAsDateTime: true)), Localizer.Format("#autoLOC_226976"), Localizer.Format("#autoLOC_226975"), Localizer.Format("#autoLOC_360842"), delegate(bool dontShowAgain)
				{
					DeleteAlarmDialogOutput(dontShowAgain, deleteAlarm: true);
				}, delegate(bool dontShowAgain)
				{
					DeleteAlarmDialogOutput(dontShowAgain, deleteAlarm: false);
				});
			}
			else
			{
				DeleteAlarmDialogOutput(!GameSettings.SHOW_DELETE_ALARM_CONFIRMATION, deleteAlarm: true);
			}
		}
		else
		{
			UnityEngine.Object.Destroy(base.gameObject);
		}
	}

	private void DeleteAlarmDialogOutput(bool dontShowAgain, bool deleteAlarm)
	{
		GameSettings.SHOW_DELETE_ALARM_CONFIRMATION = !dontShowAgain;
		GameSettings.SaveGameSettingsOnly();
		if (deleteAlarm)
		{
			AlarmClockScenario.DeleteAlarm(alarm.Id);
			UnityEngine.Object.Destroy(base.gameObject);
		}
	}
}
