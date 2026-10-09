using UnityEngine;

public class Splashdown : MonoBehaviour
{
	public float splashBurstDuration = 2.1f;

	public bool SelfDestruct;

	public GameObject[] splashBurstElements;

	private void Awake()
	{
		GetComponent<AudioSource>().volume = GameSettings.SHIP_VOLUME;
	}

	private void Start()
	{
		Invoke("endSplashBurst", splashBurstDuration);
	}

	private void endSplashBurst()
	{
		if (SelfDestruct)
		{
			Object.Destroy(base.gameObject);
			return;
		}
		int num = splashBurstElements.Length;
		while (num-- > 0)
		{
			Object.Destroy(splashBurstElements[num]);
		}
	}
}
