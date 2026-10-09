using UnityEngine;

public class ScaledSun : MonoBehaviour
{
	public static ScaledSun Instance { get; private set; }

	private void Awake()
	{
		if (Instance != null)
		{
			Object.DestroyImmediate(this);
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
