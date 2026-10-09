using UnityEngine;

public class KeepRot : MonoBehaviour
{
	private Quaternion rot;

	private void Start()
	{
		rot = base.transform.rotation;
	}

	private void LateUpdate()
	{
		base.transform.rotation = rot;
	}
}
