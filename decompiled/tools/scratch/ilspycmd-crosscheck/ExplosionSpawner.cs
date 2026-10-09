using UnityEngine;

public class ExplosionSpawner : MonoBehaviour
{
	public float spawnTime = 1f;

	public GameObject prefab;

	public bool parentToTransform;

	private float startTime;

	private bool fired;

	private void Awake()
	{
		startTime = Time.time;
		fired = false;
	}

	private void Update()
	{
		if (!fired && (Time.time - startTime) / spawnTime >= 1f)
		{
			GameObject gameObject = Object.Instantiate(prefab, base.transform.position, prefab.transform.rotation);
			if (parentToTransform)
			{
				gameObject.transform.parent = base.transform;
			}
			fired = true;
			Object.Destroy(this);
		}
	}
}
