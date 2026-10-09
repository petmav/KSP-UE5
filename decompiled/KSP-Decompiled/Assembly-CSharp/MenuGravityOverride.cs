using UnityEngine;

public class MenuGravityOverride : MonoBehaviour
{
	public float gee = 1f;

	private void Start()
	{
		Physics.gravity = new Vector3(0f, (0f - (float)PhysicsGlobals.GravitationalAcceleration) * gee, 0f);
	}
}
