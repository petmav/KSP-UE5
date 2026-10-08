using UnityEngine;

public class FXDepthCamera : MonoBehaviour
{
	public Shader ReplacementShader;

	public Camera depthCamera;

	private float startFarClipPlane = 300f;

	private float farClipPlaneBuffer = 100f;

	private void Start()
	{
		GameEvents.onGameSceneLoadRequested.Add(OnSceneSwitch);
		GameEvents.OnCameraDistanceAdjustedToFitVessel.Add(OnCameraDistanceAdjustedToFitVessel);
		depthCamera.SetReplacementShader(ReplacementShader, "RenderType");
		startFarClipPlane = depthCamera.farClipPlane;
	}

	private void OnSceneSwitch(GameScenes scene)
	{
		Object.DestroyImmediate(base.gameObject);
	}

	private void LateUpdate()
	{
		if (FlightCamera.fetch != null)
		{
			depthCamera.fieldOfView = FlightCamera.fetch.FieldOfView;
			depthCamera.farClipPlane = Mathf.Max(startFarClipPlane, FlightCamera.fetch.Distance + farClipPlaneBuffer);
		}
	}

	private void OnCameraDistanceAdjustedToFitVessel(Vessel v, float newDistance, Vector3 newBounds)
	{
		farClipPlaneBuffer = Mathf.Max(Mathf.Max(newBounds.x, newBounds.y), newBounds.z);
	}

	private void OnPreRender()
	{
		if (depthCamera.enabled)
		{
			depthCamera.SetReplacementShader(ReplacementShader, "RenderType");
		}
	}

	private void OnDestroy()
	{
		GameEvents.onGameSceneLoadRequested.Remove(OnSceneSwitch);
		GameEvents.OnCameraDistanceAdjustedToFitVessel.Remove(OnCameraDistanceAdjustedToFitVessel);
	}
}
