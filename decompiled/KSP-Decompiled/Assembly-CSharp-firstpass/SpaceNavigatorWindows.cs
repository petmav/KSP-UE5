using System;
using System.Runtime.InteropServices;
using TDx.TDxInput;
using UnityEngine;

internal class SpaceNavigatorWindows : SpaceNavigator
{
	private const float TransSensScale = 0.0001f;

	private const float RotSensScale = 0.0008f;

	private readonly Sensor _sensor;

	private readonly Device _device;

	private static SpaceNavigatorWindows _subInstance;

	public static SpaceNavigatorWindows SubInstance => _subInstance ?? (_subInstance = new SpaceNavigatorWindows());

	public override Vector3 GetTranslation()
	{
		float num = (Application.isPlaying ? PlayTransSens : TransSens[CurrentGear]);
		if (SubInstance._sensor != null)
		{
			return new Vector3((SpaceNavigator.LockTranslationX || SpaceNavigator.LockTranslationAll) ? 0f : ((float)SubInstance._sensor.Translation.X), (SpaceNavigator.LockTranslationY || SpaceNavigator.LockTranslationAll) ? 0f : ((float)SubInstance._sensor.Translation.Y), (SpaceNavigator.LockTranslationZ || SpaceNavigator.LockTranslationAll) ? 0f : (0f - (float)SubInstance._sensor.Translation.Z)) * num * 0.0001f;
		}
		return Vector3.zero;
	}

	public override Quaternion GetRotation()
	{
		float num = (Application.isPlaying ? PlayRotSens : RotSens);
		if (SubInstance._sensor != null)
		{
			return Quaternion.AngleAxis((float)SubInstance._sensor.Rotation.Angle * num * 0.0008f, new Vector3((SpaceNavigator.LockRotationX || SpaceNavigator.LockRotationAll) ? 0f : (0f - (float)SubInstance._sensor.Rotation.X), (SpaceNavigator.LockRotationY || SpaceNavigator.LockRotationAll) ? 0f : (0f - (float)SubInstance._sensor.Rotation.Y), (SpaceNavigator.LockRotationZ || SpaceNavigator.LockRotationAll) ? 0f : ((float)SubInstance._sensor.Rotation.Z)));
		}
		return Quaternion.identity;
	}

	private SpaceNavigatorWindows()
	{
		SpaceNavigator._deviceAvailable = false;
		try
		{
			if (_device == null)
			{
				_device = new DeviceClass();
				_sensor = _device.Sensor;
			}
			if (!_device.IsConnected)
			{
				_device.Connect();
			}
			SpaceNavigator._deviceAvailable = true;
		}
		catch (COMException ex)
		{
			if (string.IsNullOrEmpty(ex.Message))
			{
				Debug.Log("[SpaceNavigatorWindows]: No device found.");
			}
			else
			{
				Debug.LogWarning("[SpaceNavigatorWindows]: Could not initialize device.\n" + ex.Message);
			}
		}
	}

	public override void Dispose()
	{
		try
		{
			if (_device != null && _device.IsConnected)
			{
				_device.Disconnect();
				_subInstance = null;
				GC.Collect();
			}
		}
		catch (COMException ex)
		{
			Debug.LogError(ex.ToString());
		}
	}
}
