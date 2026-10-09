using UnityEngine;

public class DestroyAfterTime : MonoBehaviour
{
	public float delay = 1f;

	private void Start()
	{
		Invoke("Kill", delay);
	}

	private void Kill()
	{
		Object.Destroy(base.gameObject);
	}
}
