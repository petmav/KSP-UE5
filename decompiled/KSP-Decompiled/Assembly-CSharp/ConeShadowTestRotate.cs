using UnityEngine;

public class ConeShadowTestRotate : MonoBehaviour
{
	public float rotationSpeed = 1f;

	private void Update()
	{
		base.transform.Rotate(0f, Time.deltaTime * rotationSpeed, 0f);
	}
}
