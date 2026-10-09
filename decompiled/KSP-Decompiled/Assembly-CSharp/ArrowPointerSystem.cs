using UnityEngine;

public class ArrowPointerSystem : MonoBehaviour
{
	public Material material;

	public float baseSize = 0.1f;

	public static ArrowPointerSystem Instance { get; private set; }

	public static Material Material => Instance.material;

	public static float BaseSize => Instance.baseSize;

	private void Awake()
	{
		Instance = this;
	}

	private void OnDestroy()
	{
		if (Instance != null && Instance == this)
		{
			Instance = null;
		}
	}
}
