using UnityEngine;

public class ScaledFloatingOrigin : MonoBehaviour
{
	public Transform target;

	private void FixedUpdate()
	{
		base.transform.position = -base.transform.InverseTransformPoint(target.parent.position);
	}
}
