using System;
using UnityEngine;

public class AudioScreen : MonoBehaviour
{
	public UISlider spaceCraftSlider;

	public SpriteText spaceCraftTag;

	public UISlider ambienceSlider;

	public SpriteText ambienceTag;

	public UISlider uiSlider;

	public SpriteText uiTag;

	public UISlider musicSlider;

	public SpriteText musicTag;

	public UISlider voicesSlider;

	public SpriteText voicesTag;

	public UISlider thresholdSlider;

	public SpriteText thresholdTag;

	public UISlider responsivenessSlider;

	public SpriteText responsivenessTag;

	public UIStateToggleBtn normalizerBox;

	public SpriteText normalizerTag;

	private void Start()
	{
		spaceCraftSlider.AddValueChangedDelegate(audioChangedDel);
		ambienceSlider.AddValueChangedDelegate(audioChangedDel);
		uiSlider.AddValueChangedDelegate(audioChangedDel);
		musicSlider.AddValueChangedDelegate(audioChangedDel);
		voicesSlider.AddValueChangedDelegate(audioChangedDel);
		thresholdSlider.AddValueChangedDelegate(audioChangedDel);
		responsivenessSlider.AddValueChangedDelegate(audioChangedDel);
		normalizerBox.SetValueChangedDelegate(audioChangedDel);
		initUI();
	}

	private void initUI()
	{
		spaceCraftSlider.defaultValue = CFGFileOperator.shipVol;
		spaceCraftSlider.Value = CFGFileOperator.shipVol;
		ambienceSlider.defaultValue = CFGFileOperator.ambienceVol;
		ambienceSlider.Value = CFGFileOperator.ambienceVol;
		uiSlider.defaultValue = CFGFileOperator.uiVol;
		uiSlider.Value = CFGFileOperator.uiVol;
		musicSlider.defaultValue = CFGFileOperator.musicVol;
		musicSlider.Value = CFGFileOperator.musicVol;
		voicesSlider.defaultValue = CFGFileOperator.voicesVol;
		voicesSlider.Value = CFGFileOperator.voicesVol;
		thresholdSlider.defaultValue = CFGFileOperator.normalizerThreshold / 5f;
		thresholdSlider.Value = CFGFileOperator.normalizerThreshold / 5f;
		responsivenessSlider.defaultValue = CFGFileOperator.normalizerResponsiveness / 32f;
		responsivenessSlider.Value = CFGFileOperator.normalizerResponsiveness / 32f;
		if (CFGFileOperator.normalizerOnOff)
		{
			normalizerBox.SetToggleState("Checked");
			normalizerTag.Text = "Enabled";
		}
		else
		{
			normalizerBox.SetToggleState("Unchecked");
			normalizerTag.Text = "Disabled";
		}
		spaceCraftTag.Text = Math.Round(spaceCraftSlider.Value, 2).ToString("0%");
		ambienceTag.Text = Math.Round(ambienceSlider.Value, 2).ToString("0%");
		uiTag.Text = Math.Round(uiSlider.Value, 2).ToString("0%");
		musicTag.Text = Math.Round(musicSlider.Value, 2).ToString("0%");
		voicesTag.Text = Math.Round(voicesSlider.Value, 2).ToString("0%");
		thresholdTag.Text = Math.Round(thresholdSlider.Value * 5f, 2).ToString("0.0");
		responsivenessTag.Text = Math.Round(responsivenessSlider.Value * 32f, 2).ToString("0");
	}

	private void audioChangedDel(IUIObject obj)
	{
		if (obj == spaceCraftSlider)
		{
			spaceCraftTag.Text = Math.Round(spaceCraftSlider.Value, 2).ToString("0%");
		}
		else if (obj == ambienceSlider)
		{
			ambienceTag.Text = Math.Round(ambienceSlider.Value, 2).ToString("0%");
		}
		else if (obj == uiSlider)
		{
			uiTag.Text = Math.Round(uiSlider.Value, 2).ToString("0%");
		}
		else if (obj == musicSlider)
		{
			musicTag.Text = Math.Round(musicSlider.Value, 2).ToString("0%");
		}
		else if (obj == voicesSlider)
		{
			voicesTag.Text = Math.Round(voicesSlider.Value, 2).ToString("0%");
		}
		else if (obj == thresholdSlider)
		{
			thresholdTag.Text = Math.Round(thresholdSlider.Value * 5f, 2).ToString("0.0");
		}
		else if (obj == responsivenessSlider)
		{
			responsivenessTag.Text = Math.Round(responsivenessSlider.Value * 32f, 2).ToString("0");
		}
		else if (obj == normalizerBox)
		{
			if (normalizerBox.StateName == "Checked")
			{
				normalizerTag.Text = "Enabled";
			}
			else
			{
				normalizerTag.Text = "Disabled";
			}
		}
	}

	public void commitCurrentSetting()
	{
		if (normalizerBox.StateName == "Checked")
		{
			CFGFileOperator.normalizerOnOff = true;
		}
		else
		{
			CFGFileOperator.normalizerOnOff = false;
		}
		CFGFileOperator.ambienceVol = ambienceSlider.Value;
		CFGFileOperator.uiVol = uiSlider.Value;
		CFGFileOperator.musicVol = musicSlider.Value;
		CFGFileOperator.voicesVol = voicesSlider.Value;
		CFGFileOperator.shipVol = spaceCraftSlider.Value;
		CFGFileOperator.normalizerThreshold = thresholdSlider.Value * 5f;
		CFGFileOperator.normalizerResponsiveness = responsivenessSlider.Value * 32f;
	}
}
