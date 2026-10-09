using UnityEngine;

public class VolumeController : MonoBehaviour
{
	private void Start()
	{
		GetComponent<AudioSource>().pitch = Random.Range(0.8f, 1.2f);
		GetComponent<AudioSource>().volume = GameSettings.SHIP_VOLUME;
	}
}
