using UnityEngine;

public class DisableOnPlay : MonoBehaviour
{
	private void Start()
	{
		base.gameObject.SetActive(value: false);
	}
}
