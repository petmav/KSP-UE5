using System;
using System.Collections.Generic;
using UnityEngine;

public abstract class SpaceNavigator : IDisposable
{
	private static bool _lockTranslationX;

	private static bool _lockTranslationY;

	private static bool _lockTranslationZ;

	private static bool _lockTranslationAll;

	private static bool _lockRotationX;

	private static bool _lockRotationY;

	private static bool _lockRotationZ;

	private static bool _lockRotationAll;

	protected static bool _deviceAvailable;

	private int Gears = 3;

	public int CurrentGear = 1;

	public static List<float> TransSensDefault = new List<float> { 50f, 1f, 0.05f };

	public static List<float> TransSensMinDefault = new List<float> { 1f, 0.1f, 0.01f };

	public static List<float> TransSensMaxDefault = new List<float> { 100f, 10f, 1f };

	public float PlayTransSens = TransSensDefault[1];

	public List<float> TransSens = new List<float>(TransSensDefault);

	public List<float> TransSensMin = new List<float>(TransSensMinDefault);

	public List<float> TransSensMax = new List<float>(TransSensMaxDefault);

	public const float RotSensDefault = 1f;

	public const float RotSensMinDefault = 0f;

	public const float RotSensMaxDefault = 5f;

	public float PlayRotSens = 1f;

	public float RotSens = 1f;

	public float RotSensMin;

	public float RotSensMax = 5f;

	private const string TransSensKey = "Translation sensitivity";

	private const string TransSensMinKey = "Translation sensitivity minimum";

	private const string TransSensMaxKey = "Translation sensitivity maximum";

	private const string LockTranslationAllKey = "Translation lock all";

	private const string LockTranslationXKey = "Translation lock X";

	private const string LockTranslationYKey = "Translation lock Y";

	private const string LockTranslationZKey = "Translation lock Z";

	private const string RotSensKey = "Rotation sensitivity";

	private const string RotSensMinKey = "Rotation sensitivity minimum";

	private const string RotSensMaxKey = "Rotation sensitivity maximum";

	private const string LockRotationAllKey = "Rotation lock all";

	private const string LockRotationXKey = "Rotation lock X";

	private const string LockRotationYKey = "Rotation lock Y";

	private const string LockRotationZKey = "Rotation lock Z";

	private static SpaceNavigator _instance;

	public static Vector3 Translation => Instance.GetTranslation();

	public static Quaternion Rotation => Instance.GetRotation();

	public static bool LockTranslationX
	{
		get
		{
			if (_lockTranslationX)
			{
				return !Application.isPlaying;
			}
			return false;
		}
		set
		{
			_lockTranslationX = value;
		}
	}

	public static bool LockTranslationY
	{
		get
		{
			if (_lockTranslationY)
			{
				return !Application.isPlaying;
			}
			return false;
		}
		set
		{
			_lockTranslationY = value;
		}
	}

	public static bool LockTranslationZ
	{
		get
		{
			if (_lockTranslationZ)
			{
				return !Application.isPlaying;
			}
			return false;
		}
		set
		{
			_lockTranslationZ = value;
		}
	}

	public static bool LockTranslationAll
	{
		get
		{
			if (_lockTranslationAll)
			{
				return !Application.isPlaying;
			}
			return false;
		}
		set
		{
			_lockTranslationAll = value;
		}
	}

	public static bool LockRotationX
	{
		get
		{
			if (_lockRotationX)
			{
				return !Application.isPlaying;
			}
			return false;
		}
		set
		{
			_lockRotationX = value;
		}
	}

	public static bool LockRotationY
	{
		get
		{
			if (_lockRotationY)
			{
				return !Application.isPlaying;
			}
			return false;
		}
		set
		{
			_lockRotationY = value;
		}
	}

	public static bool LockRotationZ
	{
		get
		{
			if (_lockRotationZ)
			{
				return !Application.isPlaying;
			}
			return false;
		}
		set
		{
			_lockRotationZ = value;
		}
	}

	public static bool LockRotationAll
	{
		get
		{
			if (_lockRotationAll)
			{
				return !Application.isPlaying;
			}
			return false;
		}
		set
		{
			_lockRotationAll = value;
		}
	}

	public static bool Available => _deviceAvailable;

	public static SpaceNavigator Instance
	{
		get
		{
			if (_instance == null)
			{
				_instance = SpaceNavigatorWindows.SubInstance;
				if (!_deviceAvailable)
				{
					_instance = SpaceNavigatorNoDevice.SubInstance;
				}
			}
			return _instance;
		}
		set
		{
			_instance = value;
		}
	}

	public static Quaternion RotationInLocalCoordSys(Transform coordSys)
	{
		return coordSys.rotation * Rotation * Quaternion.Inverse(coordSys.rotation);
	}

	public static void SetTranslationSensitivity(float newPlayTransSens)
	{
		Instance.PlayTransSens = newPlayTransSens;
	}

	public static void SetRotationSensitivity(float newPlayRotSens)
	{
		Instance.PlayRotSens = newPlayRotSens;
	}

	public abstract Vector3 GetTranslation();

	public abstract Quaternion GetRotation();

	public abstract void Dispose();

	public void ReadSettings()
	{
		for (int i = 0; i < Gears; i++)
		{
			TransSens[i] = PlayerPrefs.GetFloat("Translation sensitivity" + i, TransSensDefault[i]);
			TransSensMin[i] = PlayerPrefs.GetFloat("Translation sensitivity minimum" + i, TransSensMinDefault[i]);
			TransSensMax[i] = PlayerPrefs.GetFloat("Translation sensitivity maximum" + i, TransSensMaxDefault[i]);
		}
		_lockTranslationAll = PlayerPrefs.GetInt("Translation lock all", 0) == 1;
		_lockTranslationX = PlayerPrefs.GetInt("Translation lock X", 0) == 1;
		_lockTranslationY = PlayerPrefs.GetInt("Translation lock Y", 0) == 1;
		_lockTranslationZ = PlayerPrefs.GetInt("Translation lock Z", 0) == 1;
		RotSens = PlayerPrefs.GetFloat("Rotation sensitivity", 1f);
		RotSensMin = PlayerPrefs.GetFloat("Rotation sensitivity minimum", 0f);
		RotSensMax = PlayerPrefs.GetFloat("Rotation sensitivity maximum", 5f);
		_lockRotationAll = PlayerPrefs.GetInt("Rotation lock all", 0) == 1;
		_lockRotationX = PlayerPrefs.GetInt("Rotation lock X", 0) == 1;
		_lockRotationY = PlayerPrefs.GetInt("Rotation lock Y", 0) == 1;
		_lockRotationZ = PlayerPrefs.GetInt("Rotation lock Z", 0) == 1;
	}

	public void WriteSettings()
	{
		for (int i = 0; i < Gears; i++)
		{
			PlayerPrefs.SetFloat("Translation sensitivity" + i, TransSens[i]);
			PlayerPrefs.SetFloat("Translation sensitivity minimum" + i, TransSensMin[i]);
			PlayerPrefs.SetFloat("Translation sensitivity maximum" + i, TransSensMax[i]);
		}
		PlayerPrefs.SetInt("Translation lock all", _lockTranslationAll ? 1 : 0);
		PlayerPrefs.SetInt("Translation lock X", _lockTranslationX ? 1 : 0);
		PlayerPrefs.SetInt("Translation lock Y", _lockTranslationY ? 1 : 0);
		PlayerPrefs.SetInt("Translation lock Z", _lockTranslationZ ? 1 : 0);
		PlayerPrefs.SetFloat("Rotation sensitivity", RotSens);
		PlayerPrefs.SetFloat("Rotation sensitivity minimum", RotSensMin);
		PlayerPrefs.SetFloat("Rotation sensitivity maximum", RotSensMax);
		PlayerPrefs.SetInt("Rotation lock all", _lockRotationAll ? 1 : 0);
		PlayerPrefs.SetInt("Rotation lock X", _lockRotationX ? 1 : 0);
		PlayerPrefs.SetInt("Rotation lock Y", _lockRotationY ? 1 : 0);
		PlayerPrefs.SetInt("Rotation lock Z", _lockRotationZ ? 1 : 0);
	}
}
