using System;
using KSP.Localization;
using KSP.UI;
using TMPro;
using UnityEngine;
using UnityEngine.UI;

public class AlarmClockUINextAlarm : MonoBehaviour
{
	[SerializeField]
	private Button deleteButton;

	public Image alarmType;

	public TextMeshProUGUI nameLabel;

	public TextMeshProUGUI timeLabel;

	public Color futureAlarmTextColor = Color.white;

	public Color pastAlarmTextColor = new Color32(103, 113, 125, byte.MaxValue);

	private Color currentTextColor;

	[SerializeField]
	private RectTransform commNetRect;

	[SerializeField]
	private RectTransform commNetBackgroundRect;

	[SerializeField]
	private RectTransform nextAlarmRect;

	private AlarmTypeBase alarm;

	private double currentUT;

	private double lastUIUpdateUT;

	private Type currentAlarmType;

	private Vector2 nextAlarmSizeDelta;

	private void Start()
	{
		deleteButton.onClick.AddListener(DeleteAlarm);
		UIUpdate_NoAlarm();
	}

	private void Update()
	{
		currentUT = Planetarium.GetUniversalTime();
		if (currentUT - lastUIUpdateUT > AlarmClockScenario.UIUpdatePeriod)
		{
			UIUpdate();
			lastUIUpdateUT = currentUT;
		}
	}

	private void UIUpdate()
	{
		if (HighLogic.LoadedSceneIsFlight && commNetRect != null)
		{
			nextAlarmSizeDelta = nextAlarmRect.sizeDelta;
			nextAlarmSizeDelta.x = Mathf.Max(240f, (commNetRect.anchoredPosition + commNetBackgroundRect.anchoredPosition + commNetBackgroundRect.sizeDelta).x);
			nextAlarmRect.sizeDelta = nextAlarmSizeDelta;
		}
		alarm = AlarmClockScenario.GetNextOrLastAlarm();
		if (alarm == null)
		{
			if (currentAlarmType != null)
			{
				UIUpdate_NoAlarm();
			}
			return;
		}
		if (currentAlarmType != alarm.GetType())
		{
			alarmType.gameObject.SetActive(value: true);
			deleteButton.gameObject.SetActive(value: true);
			timeLabel.gameObject.SetActive(value: true);
			alarmType.sprite = AlarmClockScenario.AlarmSprites[alarm.iconURL];
			currentAlarmType = alarm.GetType();
		}
		currentTextColor = ((currentUT > alarm.ut) ? pastAlarmTextColor : futureAlarmTextColor);
		nameLabel.text = alarm.title;
		nameLabel.color = currentTextColor;
		timeLabel.color = currentTextColor;
		timeLabel.text = AlarmClockUIFrame.PrintTimeStampCompact(alarm.TimeToAlarm, days: true, years: true, timeAsDateTime: true);
	}

	private void UIUpdate_NoAlarm()
	{
		alarmType.gameObject.SetActive(value: false);
		timeLabel.gameObject.SetActive(value: false);
		deleteButton.gameObject.SetActive(value: false);
		nameLabel.text = Localizer.Format("#autoLOC_8003535");
		nameLabel.color = pastAlarmTextColor;
		currentAlarmType = null;
	}

	private void OnDestroy()
	{
		deleteButton.onClick.AddListener(DeleteAlarm);
	}

	private void DeleteAlarm()
	{
		if (alarm == null)
		{
			return;
		}
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

	private void DeleteAlarmDialogOutput(bool dontShowAgain, bool deleteAlarm)
	{
		GameSettings.SHOW_DELETE_ALARM_CONFIRMATION = !dontShowAgain;
		if (deleteAlarm)
		{
			AlarmClockScenario.DeleteAlarm(alarm.Id);
		}
	}
}
