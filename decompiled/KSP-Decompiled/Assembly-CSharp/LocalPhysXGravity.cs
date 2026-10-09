using UnityEngine;

public class LocalPhysXGravity : MonoBehaviour
{
	private Transform trf;

	private void Start()
	{
		trf = base.transform;
	}

	private void FixedUpdate()
	{
		Physics.gravity = FlightGlobals.getGeeForceAtPosition(trf.position, FlightGlobals.currentMainBody);
	}
}
