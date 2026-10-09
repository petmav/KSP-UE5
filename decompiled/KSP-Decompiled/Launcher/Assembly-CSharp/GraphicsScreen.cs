using System;
using System.Collections.Generic;
using UnityEngine;

public class GraphicsScreen : MonoBehaviour
{
	public UISlider terrainDetailSlider;

	public SpriteText terrainDetailLabel;

	public UIStateToggleBtn sm3ShaderBox;

	public SpriteText sm3ShaderLabel;

	public UIStateToggleBtn terrainScatterBox;

	public SpriteText terrainScatterLabel;

	public UISlider scatterDensitySlider;

	public SpriteText scatterDensityLabel;

	public UISlider renderQSlider;

	public SpriteText renderQLabel;

	public UIButton textureQMinusBut;

	public UIButton textureQPlusBut;

	public SpriteText textureQLabel;

	public UIStateToggleBtn fallBackShaderBox;

	public SpriteText fallBackShaderLabel;

	public UISlider aeroFXSlider;

	public SpriteText aeroFXLabel;

	public UIButton screenResMinusBut;

	public UIButton screenResPlusBut;

	public SpriteText screenResLabel;

	public UIStateToggleBtn fullScreenBox;

	public SpriteText fullScreenLabel;

	public UIButton antiAliasMinusBut;

	public UIButton antiAliasPlusBut;

	public SpriteText antiAliasLabel;

	public UIButton vSyncMinusBut;

	public UIButton vSyncPlusBut;

	public SpriteText vSyncLabel;

	public UIButton frameLimitMinusBut;

	public UIButton frameLimitPlusBut;

	public SpriteText frameLimitLabel;

	public Resolution[] resolutions;

	public DynamicUIElement renderQObj;

	public DynamicUIElement textureQObj;

	public DynamicUIElement aeroFXObj;

	public DynamicUIElement screenResObj;

	public DynamicUIElement antiAliasObj;

	public DynamicUIElement vSyncObj;

	public DynamicUIElement frameLimitObj;

	public DynamicUIElement terrainDetailObj;

	public DynamicUIElement scatterDensityObj;

	private void Awake()
	{
		resolutions = Screen.resolutions;
	}

	private void Start()
	{
		terrainDetailObj = default(DynamicUIElement);
		terrainDetailObj.totalSteps = 3;
		terrainDetailObj.currentStep = 2;
		terrainDetailObj.values = new List<string>();
		terrainDetailObj.values.Add("Low");
		terrainDetailObj.values.Add("Default");
		terrainDetailObj.values.Add("High");
		scatterDensityObj = default(DynamicUIElement);
		scatterDensityObj.totalSteps = 10;
		scatterDensityObj.currentStep = 9;
		scatterDensityObj.values = new List<string>();
		for (int i = 0; i < 10; i++)
		{
			string item = Convert.ToString(i * 10) ?? "";
			scatterDensityObj.values.Add(item);
		}
		renderQObj = default(DynamicUIElement);
		renderQObj.totalSteps = 6;
		renderQObj.currentStep = 5;
		renderQObj.values = new List<string>();
		renderQObj.values.Add("Fastest");
		renderQObj.values.Add("Fast");
		renderQObj.values.Add("Simple");
		renderQObj.values.Add("Good");
		renderQObj.values.Add("Beautiful");
		renderQObj.values.Add("Fantastic");
		textureQObj = default(DynamicUIElement);
		textureQObj.totalSteps = 4;
		textureQObj.values = new List<string>();
		textureQObj.values.Add("Full Res");
		textureQObj.values.Add("Half Res");
		textureQObj.values.Add("Quarter Res");
		textureQObj.values.Add("Eight Res");
		aeroFXObj = default(DynamicUIElement);
		aeroFXObj.totalSteps = 4;
		aeroFXObj.currentStep = 3;
		aeroFXObj.values = new List<string>();
		aeroFXObj.values.Add("Minimal");
		aeroFXObj.values.Add("Very Low");
		aeroFXObj.values.Add("Low");
		aeroFXObj.values.Add("Normal");
		screenResObj = default(DynamicUIElement);
		screenResObj.totalSteps = resolutions.Length;
		screenResObj.currentStep = screenResObj.totalSteps - 1;
		screenResObj.values = new List<string>();
		Resolution[] array = resolutions;
		for (int j = 0; j < array.Length; j++)
		{
			Resolution resolution = array[j];
			screenResObj.values.Add(resolution.width + "x" + resolution.height);
		}
		antiAliasObj = default(DynamicUIElement);
		antiAliasObj.totalSteps = 4;
		antiAliasObj.currentStep = 3;
		antiAliasObj.values = new List<string>();
		antiAliasObj.values.Add("Off");
		antiAliasObj.values.Add("2x");
		antiAliasObj.values.Add("4x");
		antiAliasObj.values.Add("8x");
		vSyncObj = default(DynamicUIElement);
		vSyncObj.totalSteps = 3;
		vSyncObj.currentStep = 2;
		vSyncObj.values = new List<string>();
		vSyncObj.values.Add("Don't Sync");
		vSyncObj.values.Add("Every VBlank");
		vSyncObj.values.Add("Every Second VBlank");
		frameLimitObj = default(DynamicUIElement);
		frameLimitObj.totalSteps = 8;
		frameLimitObj.currentStep = 7;
		frameLimitObj.values = new List<string>();
		frameLimitObj.values.Add("Default");
		for (int k = 3; k < 10; k++)
		{
			string item2 = Convert.ToString(k * 20) + " FPS";
			frameLimitObj.values.Add(item2);
		}
		terrainDetailSlider.AddValueChangedDelegate(graphicsChangedDel);
		sm3ShaderBox.SetValueChangedDelegate(graphicsChangedDel);
		terrainScatterBox.SetValueChangedDelegate(graphicsChangedDel);
		scatterDensitySlider.SetValueChangedDelegate(graphicsChangedDel);
		renderQSlider.SetValueChangedDelegate(graphicsChangedDel);
		textureQMinusBut.SetValueChangedDelegate(graphicsChangedDel);
		textureQPlusBut.SetValueChangedDelegate(graphicsChangedDel);
		fallBackShaderBox.SetValueChangedDelegate(graphicsChangedDel);
		aeroFXSlider.SetValueChangedDelegate(graphicsChangedDel);
		screenResMinusBut.SetValueChangedDelegate(graphicsChangedDel);
		screenResPlusBut.SetValueChangedDelegate(graphicsChangedDel);
		fullScreenBox.SetValueChangedDelegate(graphicsChangedDel);
		antiAliasMinusBut.SetValueChangedDelegate(graphicsChangedDel);
		antiAliasPlusBut.SetValueChangedDelegate(graphicsChangedDel);
		vSyncMinusBut.SetValueChangedDelegate(graphicsChangedDel);
		vSyncPlusBut.SetValueChangedDelegate(graphicsChangedDel);
		frameLimitMinusBut.SetValueChangedDelegate(graphicsChangedDel);
		frameLimitPlusBut.SetValueChangedDelegate(graphicsChangedDel);
		setInitialUIValues();
	}

	private void setInitialUIValues()
	{
		switch (CFGFileOperator.terrainDetail)
		{
		case " Default":
			terrainDetailSlider.defaultValue = 0.5f;
			terrainDetailSlider.Value = 0.5f;
			break;
		case " Low":
			terrainDetailSlider.defaultValue = 0f;
			terrainDetailSlider.Value = 0f;
			break;
		case " High":
			terrainDetailSlider.defaultValue = 1f;
			terrainDetailSlider.Value = 1f;
			break;
		default:
			terrainDetailSlider.defaultValue = 0f;
			terrainDetailSlider.Value = 0f;
			break;
		}
		if (CFGFileOperator.sm3Terrain)
		{
			sm3ShaderBox.SetToggleState("Checked");
			sm3ShaderLabel.Text = "Enabled";
		}
		else
		{
			sm3ShaderBox.SetToggleState("Unchecked");
			sm3ShaderLabel.Text = "Disabled";
		}
		if (CFGFileOperator.terrainScatters)
		{
			terrainScatterBox.SetToggleState("Checked");
			terrainScatterLabel.Text = "Enabled";
		}
		else
		{
			terrainScatterBox.SetToggleState("Unchecked");
			terrainScatterLabel.Text = "Disabled";
		}
		if (CFGFileOperator.fallbackPartShader)
		{
			fallBackShaderBox.SetToggleState("Checked");
			fallBackShaderLabel.Text = "Enabled";
		}
		else
		{
			fallBackShaderBox.SetToggleState("Unchecked");
			fallBackShaderLabel.Text = "Disabled";
		}
		if (CFGFileOperator.fullScreen)
		{
			fullScreenBox.SetToggleState("Checked");
			fullScreenLabel.Text = "Enabled";
		}
		else
		{
			fullScreenBox.SetToggleState("Unchecked");
			fullScreenLabel.Text = "Disabled";
		}
		scatterDensitySlider.defaultValue = CFGFileOperator.scatterDensity;
		scatterDensitySlider.Value = CFGFileOperator.scatterDensity;
		scatterDensityLabel.Text = Math.Round(scatterDensitySlider.Value, 1).ToString("0%");
		renderQSlider.defaultValue = CFGFileOperator.renderQ / 5f;
		renderQSlider.Value = CFGFileOperator.renderQ / 5f;
		renderQLabel.Text = renderQObj.values[(int)CFGFileOperator.renderQ];
		aeroFXSlider.defaultValue = CFGFileOperator.aeroFXQ / 3f;
		aeroFXSlider.Value = CFGFileOperator.aeroFXQ / 3f;
		aeroFXLabel.Text = aeroFXObj.values[(int)CFGFileOperator.aeroFXQ];
		textureQLabel.Text = textureQObj.values[(int)CFGFileOperator.textureQ];
		textureQObj.currentStep = (int)CFGFileOperator.textureQ;
		if (screenResObj.values.Contains(CFGFileOperator.resolution))
		{
			screenResLabel.Text = CFGFileOperator.resolution;
			screenResObj.currentStep = screenResObj.values.IndexOf(screenResLabel.Text);
		}
		antiAliasObj.currentStep = (int)Math.Floor(CFGFileOperator.antialias / 2f);
		if (antiAliasObj.currentStep > 3)
		{
			antiAliasObj.currentStep = 3;
		}
		antiAliasLabel.Text = antiAliasObj.values[antiAliasObj.currentStep];
		vSyncObj.currentStep = (int)CFGFileOperator.vsync;
		vSyncLabel.Text = vSyncObj.values[vSyncObj.currentStep];
		if (CFGFileOperator.fpsLimit > 0f)
		{
			frameLimitObj.currentStep = (int)((double)CFGFileOperator.fpsLimit * 0.05 - 2.0);
		}
		else
		{
			frameLimitObj.currentStep = 0;
		}
		frameLimitLabel.Text = frameLimitObj.values[frameLimitObj.currentStep];
	}

	private void graphicsChangedDel(IUIObject obj)
	{
		if (obj == terrainDetailSlider)
		{
			if (terrainDetailSlider.Value >= 0f && (double)terrainDetailSlider.Value < 0.5)
			{
				terrainDetailLabel.Text = "Low";
			}
			if ((double)terrainDetailSlider.Value >= 0.5 && (double)terrainDetailSlider.Value < 0.66)
			{
				terrainDetailLabel.Text = "Default";
			}
			if ((double)terrainDetailSlider.Value > 0.66 && terrainDetailSlider.Value <= 1f)
			{
				terrainDetailLabel.Text = "High";
			}
		}
		else if (obj == sm3ShaderBox)
		{
			if (sm3ShaderBox.StateName == "Checked")
			{
				sm3ShaderLabel.Text = "Enabled";
			}
			else
			{
				sm3ShaderLabel.Text = "Disabled";
			}
		}
		else if (obj == terrainScatterBox)
		{
			if (terrainScatterBox.StateName == "Checked")
			{
				terrainScatterLabel.Text = "Enabled";
			}
			else
			{
				terrainScatterLabel.Text = "Disabled";
			}
		}
		else if (obj == fullScreenBox)
		{
			if (fullScreenBox.StateName == "Checked")
			{
				fullScreenLabel.Text = "Enabled";
			}
			else
			{
				fullScreenLabel.Text = "Disabled";
			}
		}
		else if (obj == fallBackShaderBox)
		{
			if (fallBackShaderBox.StateName == "Checked")
			{
				fallBackShaderLabel.Text = "Enabled";
			}
			else
			{
				fallBackShaderLabel.Text = "Disabled";
			}
		}
		else if (obj == textureQPlusBut)
		{
			if (textureQObj.currentStep != 0)
			{
				textureQObj.currentStep--;
				textureQLabel.Text = textureQObj.values[textureQObj.currentStep];
			}
		}
		else if (obj == textureQMinusBut)
		{
			if (textureQObj.currentStep != textureQObj.totalSteps - 1)
			{
				textureQObj.currentStep++;
				textureQLabel.Text = textureQObj.values[textureQObj.currentStep];
			}
		}
		else if (obj == screenResMinusBut)
		{
			if (screenResObj.currentStep != 0)
			{
				screenResObj.currentStep--;
				screenResLabel.Text = screenResObj.values[screenResObj.currentStep];
			}
		}
		else if (obj == screenResPlusBut)
		{
			if (screenResObj.currentStep < screenResObj.totalSteps - 1)
			{
				screenResObj.currentStep++;
				Debug.Log("Step: " + screenResObj.currentStep);
				screenResLabel.Text = screenResObj.values[screenResObj.currentStep];
			}
		}
		else if (obj == antiAliasMinusBut)
		{
			if (antiAliasObj.currentStep > 0)
			{
				antiAliasObj.currentStep--;
				Debug.Log("Step: " + screenResObj.currentStep);
				antiAliasLabel.Text = antiAliasObj.values[antiAliasObj.currentStep];
			}
		}
		else if (obj == antiAliasPlusBut)
		{
			if (antiAliasObj.currentStep < antiAliasObj.totalSteps - 1)
			{
				antiAliasObj.currentStep++;
				antiAliasLabel.Text = antiAliasObj.values[antiAliasObj.currentStep];
			}
		}
		else if (obj == vSyncMinusBut)
		{
			if (vSyncObj.currentStep != 0)
			{
				vSyncObj.currentStep--;
				vSyncLabel.Text = vSyncObj.values[vSyncObj.currentStep];
			}
		}
		else if (obj == vSyncPlusBut)
		{
			if (vSyncObj.currentStep != vSyncObj.totalSteps - 1)
			{
				vSyncObj.currentStep++;
				vSyncLabel.Text = vSyncObj.values[vSyncObj.currentStep];
			}
		}
		else if (obj == frameLimitMinusBut)
		{
			if (frameLimitObj.currentStep != 0)
			{
				frameLimitObj.currentStep--;
				frameLimitLabel.Text = frameLimitObj.values[frameLimitObj.currentStep];
			}
		}
		else if (obj == frameLimitPlusBut)
		{
			if (frameLimitObj.currentStep != frameLimitObj.totalSteps - 1)
			{
				frameLimitObj.currentStep++;
				frameLimitLabel.Text = frameLimitObj.values[frameLimitObj.currentStep];
			}
		}
		else if (obj == scatterDensitySlider)
		{
			scatterDensityLabel.Text = Math.Round(scatterDensitySlider.Value, 1).ToString("0%");
		}
		else if (obj == renderQSlider)
		{
			int index = (int)Math.Round(renderQSlider.Value * 5f);
			renderQLabel.Text = renderQObj.values[index];
		}
		else if (obj == aeroFXSlider)
		{
			int index2 = (int)Math.Round(aeroFXSlider.Value * 3f);
			aeroFXLabel.Text = aeroFXObj.values[index2];
		}
	}

	public void commitCurrentSetting()
	{
		if (terrainDetailSlider.Value > 0f && terrainDetailSlider.Value < 0.49f)
		{
			CFGFileOperator.terrainDetail = " Low";
		}
		else if (terrainDetailSlider.Value >= 0.5f && terrainDetailSlider.Value < 1f)
		{
			CFGFileOperator.terrainDetail = " Default";
		}
		else
		{
			CFGFileOperator.terrainDetail = " High";
		}
		if (sm3ShaderBox.StateName == "Checked")
		{
			CFGFileOperator.sm3Terrain = true;
		}
		else
		{
			CFGFileOperator.sm3Terrain = false;
		}
		if (terrainScatterBox.StateName == "Checked")
		{
			CFGFileOperator.terrainScatters = true;
		}
		else
		{
			CFGFileOperator.terrainScatters = false;
		}
		if (fullScreenBox.StateName == "Checked")
		{
			CFGFileOperator.fullScreen = true;
		}
		else
		{
			CFGFileOperator.fullScreen = false;
		}
		if (fallBackShaderBox.StateName == "Checked")
		{
			CFGFileOperator.fallbackPartShader = true;
		}
		else
		{
			CFGFileOperator.fallbackPartShader = false;
		}
		CFGFileOperator.scatterDensity = scatterDensitySlider.Value;
		CFGFileOperator.renderQ = renderQSlider.Value * 5f;
		CFGFileOperator.aeroFXQ = aeroFXSlider.Value * 3f;
		CFGFileOperator.textureQ = textureQObj.currentStep;
		CFGFileOperator.resolution = screenResObj.values[screenResObj.currentStep];
		CFGFileOperator.vsync = vSyncObj.currentStep;
		CFGFileOperator.antialias = antiAliasObj.currentStep * 2;
		if (CFGFileOperator.antialias == 6f)
		{
			CFGFileOperator.antialias = 8f;
		}
		else if (CFGFileOperator.antialias == 0f)
		{
			CFGFileOperator.antialias = 1f;
		}
		if (frameLimitObj.currentStep == 0)
		{
			CFGFileOperator.fpsLimit = -1f;
		}
		else
		{
			CFGFileOperator.fpsLimit = (frameLimitObj.currentStep + 2) * 20;
		}
	}

	public void setGraphicsProfile(string profile)
	{
		switch (profile)
		{
		case "MINIMUM":
			terrainDetailSlider.Value = 0f;
			sm3ShaderBox.SetToggleState("Unchecked");
			terrainScatterBox.SetToggleState("Unchecked");
			terrainDetailSlider.Value = 0f;
			renderQSlider.Value = 0f;
			textureQObj.currentStep = textureQObj.values.Count - 1;
			textureQLabel.Text = textureQObj.values[textureQObj.currentStep];
			fallBackShaderBox.SetToggleState("Checked");
			aeroFXSlider.Value = 0f;
			antiAliasObj.currentStep = 0;
			antiAliasLabel.Text = antiAliasObj.values[antiAliasObj.currentStep];
			vSyncObj.currentStep = 0;
			vSyncLabel.Text = vSyncObj.values[vSyncObj.currentStep];
			frameLimitObj.currentStep = 0;
			frameLimitLabel.Text = frameLimitObj.values[frameLimitObj.currentStep];
			break;
		case "LOW":
			terrainDetailSlider.Value = 0f;
			sm3ShaderBox.SetToggleState("Unchecked");
			terrainScatterBox.SetToggleState("Unchecked");
			scatterDensitySlider.Value = 0f;
			renderQSlider.Value = 0.4f;
			textureQObj.currentStep = textureQObj.values.Count - 1;
			textureQLabel.Text = textureQObj.values[textureQObj.currentStep];
			fallBackShaderBox.SetToggleState("Checked");
			aeroFXSlider.Value = 0.33f;
			antiAliasObj.currentStep = 0;
			antiAliasLabel.Text = antiAliasObj.values[antiAliasObj.currentStep];
			vSyncObj.currentStep = 0;
			vSyncLabel.Text = vSyncObj.values[vSyncObj.currentStep];
			frameLimitObj.currentStep = 1;
			frameLimitLabel.Text = frameLimitObj.values[frameLimitObj.currentStep];
			break;
		case "NORMAL":
			terrainDetailSlider.Value = 0.6f;
			sm3ShaderBox.SetToggleState("Checked");
			terrainScatterBox.SetToggleState("Unchecked");
			scatterDensitySlider.Value = 0f;
			renderQSlider.Value = 0.6f;
			textureQObj.currentStep = textureQObj.values.Count - 2;
			textureQLabel.Text = textureQObj.values[textureQObj.currentStep];
			fallBackShaderBox.SetToggleState("Unchecked");
			aeroFXSlider.Value = 0.66f;
			antiAliasObj.currentStep = 1;
			antiAliasLabel.Text = antiAliasObj.values[antiAliasObj.currentStep];
			vSyncObj.currentStep = vSyncObj.values.Count - 1;
			vSyncLabel.Text = vSyncObj.values[vSyncObj.currentStep];
			frameLimitObj.currentStep = (int)Math.Floor((float)frameLimitObj.totalSteps / 2f);
			frameLimitLabel.Text = frameLimitObj.values[frameLimitObj.currentStep];
			break;
		case "HIGH":
			terrainDetailSlider.Value = 1f;
			sm3ShaderBox.SetToggleState("Checked");
			terrainScatterBox.SetToggleState("Checked");
			scatterDensitySlider.Value = 0.5f;
			renderQSlider.Value = 0.8f;
			textureQObj.currentStep = textureQObj.values.Count - 3;
			textureQLabel.Text = textureQObj.values[textureQObj.currentStep];
			fallBackShaderBox.SetToggleState("Unchecked");
			aeroFXSlider.Value = 1f;
			antiAliasObj.currentStep = 2;
			antiAliasLabel.Text = antiAliasObj.values[antiAliasObj.currentStep];
			vSyncObj.currentStep = vSyncObj.values.Count - 1;
			vSyncLabel.Text = vSyncObj.values[vSyncObj.currentStep];
			frameLimitObj.currentStep = (int)Math.Floor((float)(2 * frameLimitObj.totalSteps) / 3f);
			frameLimitLabel.Text = frameLimitObj.values[frameLimitObj.currentStep];
			break;
		case "MAXIMUM":
			terrainDetailSlider.Value = 1f;
			sm3ShaderBox.SetToggleState("Checked");
			terrainScatterBox.SetToggleState("Checked");
			scatterDensitySlider.Value = 1f;
			renderQSlider.Value = 1f;
			textureQObj.currentStep = 0;
			textureQLabel.Text = textureQObj.values[textureQObj.currentStep];
			fallBackShaderBox.SetToggleState("Unchecked");
			aeroFXSlider.Value = 1f;
			antiAliasObj.currentStep = antiAliasObj.values.Count - 1;
			antiAliasLabel.Text = antiAliasObj.values[antiAliasObj.currentStep];
			vSyncObj.currentStep = vSyncObj.values.Count - 2;
			vSyncLabel.Text = vSyncObj.values[vSyncObj.currentStep];
			frameLimitObj.currentStep = frameLimitObj.values.Count - 1;
			frameLimitLabel.Text = frameLimitObj.values[frameLimitObj.currentStep];
			break;
		}
	}
}
