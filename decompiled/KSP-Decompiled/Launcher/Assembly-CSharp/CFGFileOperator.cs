using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using UnityEngine;

public class CFGFileOperator : MonoBehaviour
{
	public static string terrainDetail;

	public static bool sm3Terrain;

	public static bool terrainScatters;

	public static float scatterDensity;

	public static float renderQ;

	public static float textureQ;

	public static bool fallbackPartShader;

	public static float aeroFXQ;

	public static string resolution;

	public static bool fullScreen;

	public static float antialias;

	public static float vsync;

	public static float fpsLimit;

	public static float shipVol;

	public static float ambienceVol;

	public static float uiVol;

	public static float musicVol;

	public static float voicesVol;

	public static float normalizerThreshold;

	public static float normalizerResponsiveness;

	public static bool normalizerOnOff;

	private List<string> cfgFileLines;

	private Dictionary<string, string> settingsDict;

	private string settingsPath;

	private string bakPath;

	private void Start()
	{
		settingsDict = new Dictionary<string, string>();
		settingsPath = ((Application.platform == RuntimePlatform.OSXPlayer) ? Path.Combine(Application.dataPath, "../../settings.cfg") : Path.Combine(Application.dataPath, "../settings.cfg"));
		bakPath = ((Application.platform == RuntimePlatform.WindowsEditor) ? Path.Combine(Application.dataPath, "../Settings_Data/settings.cfg.bak") : Path.Combine(Application.dataPath, "settings.cfg.bak"));
		if (File.Exists(settingsPath))
		{
			try
			{
				File.Copy(settingsPath, bakPath, overwrite: true);
				Debug.Log("Backup file overwritten");
			}
			catch (IOException ex)
			{
				Debug.Log("Could not overwrite backup file, reason " + ex.Message);
			}
			cfgFileLines = File.ReadAllLines(settingsPath).OfType<string>().ToList();
			if (!checkSettingsFile())
			{
				createSettingsFile();
				return;
			}
			int num = 0;
			foreach (string cfgFileLine in cfgFileLines)
			{
				num++;
				if (cfgFileLine.StartsWith("PITCH_DOWN"))
				{
					break;
				}
				if (!cfgFileLine.StartsWith("//") && !(cfgFileLine == ""))
				{
					int num2 = cfgFileLine.IndexOf("=");
					if (num2 > 0)
					{
						string key = cfgFileLine.Substring(0, num2);
						string value = cfgFileLine.Substring(num2 + 1, cfgFileLine.Length - num2 - 1);
						settingsDict.Add(key, value);
					}
				}
			}
			cfgFileLines.RemoveRange(0, num - 1);
		}
		else
		{
			createSettingsFile();
		}
		initializeUIVariables();
	}

	private void createSettingsFile()
	{
		cfgFileLines = File.ReadAllLines(bakPath).OfType<string>().ToList();
		if (!checkSettingsFile())
		{
			Debug.Log("Incompatible backup file");
			return;
		}
		int num = 0;
		foreach (string cfgFileLine in cfgFileLines)
		{
			num++;
			if (cfgFileLine.StartsWith("PITCH_DOWN"))
			{
				break;
			}
			if (!cfgFileLine.StartsWith("//") && !(cfgFileLine == ""))
			{
				int num2 = cfgFileLine.IndexOf("=");
				if (num2 > 0)
				{
					string key = cfgFileLine.Substring(0, num2);
					string value = cfgFileLine.Substring(num2 + 1, cfgFileLine.Length - num2 - 1);
					settingsDict.Add(key, value);
				}
			}
		}
		cfgFileLines.RemoveRange(0, num - 1);
		writeSettingsFile();
	}

	private void writeSettingsFile()
	{
		string[] array = new string[cfgFileLines.Count + settingsDict.Count + 2];
		array[0] = "// KSP Game Settings";
		array[1] = "";
		int num = 2;
		foreach (KeyValuePair<string, string> item in settingsDict)
		{
			array[num] = item.Key + "=" + item.Value;
			num++;
		}
		foreach (string cfgFileLine in cfgFileLines)
		{
			array[num] = cfgFileLine;
			num++;
		}
		File.WriteAllLines(settingsPath, array);
	}

	public void saveSettings()
	{
		int num = cfgFileLines.IndexOf("TERRAIN");
		num += 2;
		cfgFileLines[num] = "\tpreset =" + terrainDetail;
		Debug.Log("Terrain detail now: " + cfgFileLines[num]);
		if (settingsDict.ContainsKey("PLANET_FORCE_SHADER_MODEL_2_0 "))
		{
			sm3Terrain = !sm3Terrain;
			settingsDict["PLANET_FORCE_SHADER_MODEL_2_0 "] = sm3Terrain.ToString();
		}
		if (settingsDict.ContainsKey("PLANET_SCATTER "))
		{
			settingsDict["PLANET_SCATTER "] = terrainScatters.ToString();
		}
		if (settingsDict.ContainsKey("PLANET_SCATTER_FACTOR "))
		{
			settingsDict["PLANET_SCATTER_FACTOR "] = scatterDensity.ToString("0.0");
		}
		if (settingsDict.ContainsKey("QUALITY_PRESET "))
		{
			settingsDict["QUALITY_PRESET "] = renderQ.ToString("0");
		}
		if (settingsDict.ContainsKey("TEXTURE_QUALITY "))
		{
			settingsDict["TEXTURE_QUALITY "] = textureQ.ToString("0");
		}
		if (settingsDict.ContainsKey("FALLBACK_PART_SHADERS "))
		{
			settingsDict["FALLBACK_PART_SHADERS "] = fallbackPartShader.ToString();
		}
		if (settingsDict.ContainsKey("AERO_FX_QUALITY "))
		{
			settingsDict["AERO_FX_QUALITY "] = aeroFXQ.ToString("0");
		}
		if (settingsDict.ContainsKey("SCREEN_RESOLUTION_WIDTH ") && settingsDict.ContainsKey("SCREEN_RESOLUTION_HEIGHT "))
		{
			settingsDict["SCREEN_RESOLUTION_WIDTH "] = resolution.Substring(0, resolution.IndexOf("x"));
			settingsDict["SCREEN_RESOLUTION_HEIGHT "] = resolution.Substring(resolution.IndexOf("x") + 1, resolution.Length - resolution.IndexOf("x") - 1);
		}
		if (settingsDict.ContainsKey("FULLSCREEN "))
		{
			settingsDict["FULLSCREEN "] = fullScreen.ToString();
		}
		if (settingsDict.ContainsKey("ANTI_ALIASING "))
		{
			settingsDict["ANTI_ALIASING "] = antialias.ToString("0");
		}
		if (settingsDict.ContainsKey("SYNC_VBL "))
		{
			settingsDict["SYNC_VBL "] = vsync.ToString("0");
		}
		if (settingsDict.ContainsKey("FRAMERATE_LIMIT "))
		{
			settingsDict["FRAMERATE_LIMIT "] = fpsLimit.ToString("0");
		}
		if (settingsDict.ContainsKey("SHIP_VOLUME "))
		{
			settingsDict["SHIP_VOLUME "] = shipVol.ToString();
		}
		if (settingsDict.ContainsKey("AMBIENCE_VOLUME "))
		{
			settingsDict["AMBIENCE_VOLUME "] = ambienceVol.ToString();
		}
		if (settingsDict.ContainsKey("MUSIC_VOLUME "))
		{
			settingsDict["MUSIC_VOLUME "] = musicVol.ToString();
		}
		if (settingsDict.ContainsKey("UI_VOLUME "))
		{
			settingsDict["UI_VOLUME "] = uiVol.ToString();
		}
		if (settingsDict.ContainsKey("VOICE_VOLUME "))
		{
			settingsDict["VOICE_VOLUME "] = voicesVol.ToString();
		}
		if (settingsDict.ContainsKey("SOUND_NORMALIZER_ENABLED "))
		{
			settingsDict["SOUND_NORMALIZER_ENABLED "] = normalizerOnOff.ToString();
		}
		if (settingsDict.ContainsKey("SOUND_NORMALIZER_THRESHOLD "))
		{
			settingsDict["SOUND_NORMALIZER_THRESHOLD "] = normalizerThreshold.ToString();
		}
		if (settingsDict.ContainsKey("SOUND_NORMALIZER_RESPONSIVENESS "))
		{
			settingsDict["SOUND_NORMALIZER_RESPONSIVENESS "] = normalizerResponsiveness.ToString();
		}
		writeSettingsFile();
	}

	private void initializeUIVariables()
	{
		int num = cfgFileLines.IndexOf("TERRAIN");
		num += 2;
		terrainDetail = cfgFileLines[num].Substring(cfgFileLines[num].IndexOf('=') + 1, cfgFileLines[num].Length - cfgFileLines[num].IndexOf('=') - 1);
		if (settingsDict.ContainsKey("PLANET_FORCE_SHADER_MODEL_2_0 "))
		{
			sm3Terrain = Convert.ToBoolean(settingsDict["PLANET_FORCE_SHADER_MODEL_2_0 "]);
			sm3Terrain = !sm3Terrain;
		}
		if (settingsDict.ContainsKey("PLANET_SCATTER "))
		{
			terrainScatters = Convert.ToBoolean(settingsDict["PLANET_SCATTER "]);
		}
		if (settingsDict.ContainsKey("PLANET_SCATTER_FACTOR "))
		{
			scatterDensity = float.Parse(settingsDict["PLANET_SCATTER_FACTOR "]);
		}
		if (settingsDict.ContainsKey("QUALITY_PRESET "))
		{
			renderQ = float.Parse(settingsDict["QUALITY_PRESET "]);
		}
		if (settingsDict.ContainsKey("TEXTURE_QUALITY "))
		{
			textureQ = float.Parse(settingsDict["TEXTURE_QUALITY "]);
		}
		if (settingsDict.ContainsKey("FALLBACK_PART_SHADERS "))
		{
			fallbackPartShader = Convert.ToBoolean(settingsDict["FALLBACK_PART_SHADERS "]);
		}
		if (settingsDict.ContainsKey("AERO_FX_QUALITY "))
		{
			aeroFXQ = float.Parse(settingsDict["AERO_FX_QUALITY "]);
		}
		if (settingsDict.ContainsKey("SCREEN_RESOLUTION_WIDTH ") && settingsDict.ContainsKey("SCREEN_RESOLUTION_HEIGHT "))
		{
			resolution = settingsDict["SCREEN_RESOLUTION_WIDTH "].Trim() + "x" + settingsDict["SCREEN_RESOLUTION_HEIGHT "].Trim();
		}
		if (settingsDict.ContainsKey("FULLSCREEN "))
		{
			fullScreen = Convert.ToBoolean(settingsDict["FULLSCREEN "]);
		}
		if (settingsDict.ContainsKey("ANTI_ALIASING "))
		{
			antialias = float.Parse(settingsDict["ANTI_ALIASING "]);
		}
		if (settingsDict.ContainsKey("SYNC_VBL "))
		{
			vsync = float.Parse(settingsDict["SYNC_VBL "]);
		}
		if (settingsDict.ContainsKey("FRAMERATE_LIMIT "))
		{
			fpsLimit = float.Parse(settingsDict["FRAMERATE_LIMIT "]);
		}
		if (settingsDict.ContainsKey("SHIP_VOLUME "))
		{
			shipVol = float.Parse(settingsDict["SHIP_VOLUME "]);
		}
		if (settingsDict.ContainsKey("AMBIENCE_VOLUME "))
		{
			ambienceVol = float.Parse(settingsDict["AMBIENCE_VOLUME "]);
		}
		if (settingsDict.ContainsKey("MUSIC_VOLUME "))
		{
			musicVol = float.Parse(settingsDict["MUSIC_VOLUME "]);
		}
		if (settingsDict.ContainsKey("UI_VOLUME "))
		{
			uiVol = float.Parse(settingsDict["UI_VOLUME "]);
		}
		if (settingsDict.ContainsKey("VOICE_VOLUME "))
		{
			voicesVol = float.Parse(settingsDict["VOICE_VOLUME "]);
		}
		if (settingsDict.ContainsKey("SOUND_NORMALIZER_ENABLED "))
		{
			normalizerOnOff = Convert.ToBoolean(settingsDict["SOUND_NORMALIZER_ENABLED "]);
		}
		if (settingsDict.ContainsKey("SOUND_NORMALIZER_THRESHOLD "))
		{
			normalizerThreshold = float.Parse(settingsDict["SOUND_NORMALIZER_THRESHOLD "]);
		}
		if (settingsDict.ContainsKey("SOUND_NORMALIZER_RESPONSIVENESS "))
		{
			normalizerResponsiveness = float.Parse(settingsDict["SOUND_NORMALIZER_RESPONSIVENESS "]);
		}
	}

	public bool checkSettingsFile()
	{
		for (int i = 0; i < 20; i++)
		{
			if (cfgFileLines[i].Contains("SETTINGS_FILE_VERSION = 1"))
			{
				return true;
			}
		}
		return false;
	}
}
