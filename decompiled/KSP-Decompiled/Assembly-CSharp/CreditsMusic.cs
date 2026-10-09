using UnityEngine;

public class CreditsMusic : MonoBehaviour
{
	private void Awake()
	{
		GetComponent<AudioSource>().volume = GameSettings.MUSIC_VOLUME;
	}
}
