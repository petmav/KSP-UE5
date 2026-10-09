using UnityEngine;

[RequireComponent(typeof(Camera))]
public class GalaxyCameraControl : MonoBehaviour
{
	public static GalaxyCameraControl Instance;

	public float defaultFoV = 60f;

	private Camera _camera;

	private void Awake()
	{
		Instance = this;
		this.GetComponentCached(ref _camera).fieldOfView = defaultFoV;
	}

	private void OnDestroy()
	{
		if (Instance != null && Instance == this)
		{
			Instance = null;
		}
	}

	public void SetFoV(float FoV)
	{
		this.GetComponentCached(ref _camera).fieldOfView = FoV;
	}
}
