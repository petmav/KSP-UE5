using UnityEngine;

public class WindmillAnimScript : MonoBehaviour
{
	public float rotationSpeed = 0.001f;

	public Vector3 rotationAxis;

	public PQSCity2 ReferenceCity;

	private bool paused;

	private void Start()
	{
		rotationSpeed = Random.Range(0.5f, 1f);
		GameEvents.onGamePause.Add(onPause);
		GameEvents.onGameUnpause.Add(onUnPause);
	}

	private void OnDestroy()
	{
		GameEvents.onGamePause.Remove(onPause);
		GameEvents.onGameUnpause.Remove(onUnPause);
	}

	private void onPause()
	{
		paused = true;
	}

	private void onUnPause()
	{
		paused = false;
	}

	private void Update()
	{
		if (ReferenceCity == null || (ReferenceCity != null && ReferenceCity.InVisibleRange))
		{
			if (FlightDriver.fetch != null)
			{
				paused = FlightDriver.Pause;
			}
			if (!paused)
			{
				base.transform.Rotate(rotationAxis, rotationSpeed);
			}
		}
	}
}
