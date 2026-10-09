using UnityEngine;

public class ExplosionDestroyer : MonoBehaviour
{
	public float explosionDuration = 1f;

	private float startTime;

	private void Awake()
	{
		startTime = Time.time;
	}

	private void Update()
	{
		if ((Time.time - startTime) / explosionDuration >= 1f)
		{
			Object.Destroy(base.gameObject);
		}
	}
}
