using UnityEngine;

public class SimpleOrbitController : MonoBehaviour
{
	public Transform satellite;

	public double distance;

	public double period;

	private double orbitalSpeed;

	private Vector3d orbitalPosition;

	private double orbitalAngle;

	private QuaternionD orbitalRot;

	private void Start()
	{
		orbitalPosition = Vector3d.right * distance;
	}

	private void FixedUpdate()
	{
		orbitalSpeed = 360.0 / period * (double)Time.fixedDeltaTime;
		orbitalRot = QuaternionD.AngleAxis(orbitalSpeed, Vector3d.up);
		orbitalPosition = orbitalRot * orbitalPosition;
		satellite.position = base.transform.position + orbitalPosition;
	}
}
