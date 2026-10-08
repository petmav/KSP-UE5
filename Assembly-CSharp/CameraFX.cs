using CameraFXModules;
using UnityEngine;

public class CameraFX : MonoBehaviour
{
	public static CameraFX Instance;

	public CameraFXCollection FX;

	private void Awake()
	{
		if ((bool)Instance)
		{
			Object.Destroy(this);
		}
		else
		{
			Instance = this;
		}
	}

	private void OnDestroy()
	{
		if (Instance != null && Instance == this)
		{
			Instance = null;
		}
	}
}
