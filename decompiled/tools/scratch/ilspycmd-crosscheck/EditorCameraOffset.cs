using UnityEngine;

[RequireComponent(typeof(CameraOffCenter))]
public class EditorCameraOffset : MonoBehaviour
{
	public float SidebarPixelWidth = 256f;

	private float sidebarScreenWidth;

	private CameraOffCenter cameraOffCenter;

	private void Start()
	{
		cameraOffCenter = GetComponent<CameraOffCenter>();
	}

	private void Update()
	{
		sidebarScreenWidth = SidebarPixelWidth / (float)Screen.width;
		cameraOffCenter.x = sidebarScreenWidth;
		cameraOffCenter.y = 0f;
	}
}
