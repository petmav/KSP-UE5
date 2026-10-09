using UnityEngine;

public class Rotato : MonoBehaviour
{
	public float speed = 0.25f;

	private void Start()
	{
	}

	private void FixedUpdate()
	{
		base.transform.Rotate(0f, speed, 0f);
	}
}
