using System;
using UnityEngine;

public class SpaceCenterCamera : MonoBehaviour
{
	public enum CameraMode
	{
		Rover,
		Overview
	}

	public string pqsName;

	public string initialPositionTransformName;

	public CameraMode mode = CameraMode.Overview;

	public KeyCode keyModeSwitch = KeyCode.Space;

	public KeyCode keyReset = KeyCode.Backspace;

	public double rotationInitial = 90.0;

	public double rotateSpeed = 90.0;

	public float mouseSpeed = 90f;

	public float elevationSpeed = 90f;

	public float elevationInitial = 45f;

	public float elevationMin;

	public float elevationMax = 85f;

	public float zoomSpeed = 90f;

	public float zoomInitial = 45f;

	public float zoomMin;

	public float zoomMax = 1000f;

	private double altitudeMin = 10.0;

	private double altitudeMax = 10.0;

	public double altitudeInitial = 10.0;

	private PQS pqs;

	private Transform initialPosition;

	private Transform cameraTransform;

	private double speed = 1.0;

	private float mousePitch;

	private float zoom = 10f;

	private double altitude = 10.0;

	private double rotationAngle;

	private float elevationAngle;

	private double radius;

	private Vector3d direction = Vector3.one;

	private QuaternionD rotation = Quaternion.identity;

	private Vector3d realPos = Vector3d.one;

	private static double Deg2Rad = Math.PI / 180.0;

	private void Reset()
	{
		rotateSpeed = 90.0;
		mouseSpeed = 90f;
	}

	private void Start()
	{
		PQS[] array = (PQS[])UnityEngine.Object.FindObjectsOfType(typeof(PQS));
		foreach (PQS pQS in array)
		{
			if (pQS.gameObject.name == pqsName)
			{
				pqs = pQS;
				break;
			}
		}
		if (pqs == null)
		{
			Debug.LogError("SpaceCenterCamera: Cannot find PQS of name '" + pqsName + "'");
			return;
		}
		initialPosition = pqs.transform.Find(initialPositionTransformName);
		if (initialPosition == null)
		{
			Debug.LogError("SpaceCenterCamera: Cannot find transform of name '" + initialPositionTransformName + "'");
			return;
		}
		radius = pqs.radius + altitudeInitial;
		base.transform.NestToParent(pqs.transform);
		cameraTransform = new GameObject("CameraTransform").transform;
		cameraTransform.NestToParent(base.transform);
		FlightCamera.fetch.transform.NestToParent(cameraTransform);
		FlightCamera.fetch.updateActive = false;
		FlightCamera.fetch.gameObject.SetActive(value: true);
		ResetCamera();
	}

	private void Update()
	{
		if (!(pqs == null))
		{
			direction = new Vector3d(Math.Sin(rotationAngle), Math.Cos(rotationAngle));
			if (Input.GetKeyDown(keyModeSwitch))
			{
				SwitchMode();
			}
			InputSpeedFactor();
			InputMovement();
			InputCamera();
			UpdateTransform();
		}
	}

	public void SetPosition(Vector3 position)
	{
		rotation = Quaternion.LookRotation(position.normalized);
		UpdateTransform();
	}

	public void SwitchMode()
	{
		if (mode == CameraMode.Rover)
		{
			mode = CameraMode.Overview;
		}
		else
		{
			mode = CameraMode.Rover;
		}
	}

	public void ResetCamera()
	{
		altitude = altitudeInitial;
		elevationAngle = elevationInitial;
		zoom = zoomInitial;
		rotationAngle = rotationInitial;
		SetPosition(pqs.transform.InverseTransformPoint(initialPosition.position));
	}

	private void InputSpeedFactor()
	{
		if (Input.GetKeyDown(KeyCode.Alpha0))
		{
			speed = 0.1;
		}
		if (Input.GetKeyDown(KeyCode.Alpha1))
		{
			speed = 1.0;
		}
		if (Input.GetKeyDown(KeyCode.Alpha2))
		{
			speed = 10.0;
		}
		if (Input.GetKeyDown(KeyCode.Alpha3))
		{
			speed = 100.0;
		}
		if (Input.GetKeyDown(KeyCode.Alpha4))
		{
			speed = 1000.0;
		}
		if (Input.GetKeyDown(KeyCode.Alpha5))
		{
			speed = 10000.0;
		}
		if (Input.GetKeyDown(KeyCode.Alpha6))
		{
			speed = 100000.0;
		}
		if (Input.GetKeyDown(KeyCode.Alpha7))
		{
			speed = 1000000.0;
		}
		if (Input.GetKeyDown(KeyCode.Alpha8))
		{
			speed = 10000000.0;
		}
		if (Input.GetKeyDown(KeyCode.Alpha9))
		{
			speed = 100000000.0;
		}
	}

	private void InputMovement()
	{
		if (Input.GetKey(KeyCode.UpArrow) || Input.GetKey(KeyCode.W))
		{
			Translate(0.0, GetSpeed());
		}
		if (Input.GetKey(KeyCode.DownArrow) || Input.GetKey(KeyCode.S))
		{
			Translate(0.0, 0.0 - GetSpeed());
		}
		if (Input.GetKey(KeyCode.LeftArrow) || Input.GetKey(KeyCode.A))
		{
			Translate(GetSpeed(), 0.0);
		}
		if (Input.GetKey(KeyCode.RightArrow) || Input.GetKey(KeyCode.D))
		{
			Translate(0.0 - GetSpeed(), 0.0);
		}
		if (Input.GetKey(KeyCode.PageUp) || Input.GetKey(KeyCode.C))
		{
			Altitude(GetSpeed() / 1000.0);
		}
		if (Input.GetKey(KeyCode.PageDown) || Input.GetKey(KeyCode.Z))
		{
			Altitude((0.0 - GetSpeed()) / 1000.0);
		}
		if (Input.GetKey(KeyCode.X))
		{
			altitude = 10.0;
		}
	}

	private void InputCamera()
	{
		if (!Input.GetMouseButton(1) && !Input.GetMouseButton(2))
		{
			Cursor.lockState = CursorLockMode.None;
			return;
		}
		Cursor.lockState = CursorLockMode.Locked;
		if (Input.GetMouseButton(1))
		{
			Rotate((double)(0f - Input.GetAxis("Mouse X")) * rotateSpeed);
			if (mode == CameraMode.Overview)
			{
				Elevate((0f - Input.GetAxis("Mouse Y")) * elevationSpeed);
			}
			else
			{
				Pitch((0f - Input.GetAxis("Mouse Y")) * elevationSpeed);
			}
		}
		else if (Input.GetMouseButton(2))
		{
			Zoom(Input.GetAxis("Mouse Y") * (0f - zoomSpeed));
		}
	}

	private double GetSpeed()
	{
		return speed * (Input.GetKey(KeyCode.LeftShift) ? 10.0 : 1.0) * (Input.GetKey(KeyCode.LeftControl) ? 0.1 : 1.0) * 1000.0;
	}

	private void Pitch(float amount)
	{
		mousePitch += amount * Time.deltaTime;
		mousePitch = Mathf.Clamp(mousePitch, -85f, 85f);
	}

	private void Altitude(double amount)
	{
		altitude += amount * (double)Time.deltaTime;
		altitude = Math.Min(altitude, altitudeMax);
		altitude = Math.Max(altitude, altitudeMin);
	}

	private void Rotate(double amount)
	{
		rotationAngle += amount * Deg2Rad * (double)Time.deltaTime;
	}

	private void Elevate(float amount)
	{
		elevationAngle += amount * Time.deltaTime;
		elevationAngle = Math.Min(elevationAngle, elevationMax);
		elevationAngle = Math.Max(elevationAngle, elevationMin);
	}

	private void Zoom(float amount)
	{
		zoom += amount * Time.deltaTime;
		zoom = Math.Min(zoom, zoomMax);
		zoom = Math.Max(zoom, zoomMin);
	}

	private void Translate(double x, double y)
	{
		QuaternionD quaternionD = QuaternionD.AngleAxis(axis: new Vector3d(0.0 - direction.y, direction.x), angle: y * (double)Time.deltaTime / radius);
		QuaternionD quaternionD2 = QuaternionD.AngleAxis(x * (double)Time.deltaTime / radius, direction);
		rotation *= quaternionD2 * quaternionD;
	}

	private void UpdateTransform()
	{
		if (mode == CameraMode.Rover)
		{
			UpdateTransformRover();
		}
		else
		{
			UpdateTransformOverview();
		}
	}

	private void UpdateTransformRover()
	{
		realPos = rotation * Vector3d.forward;
		base.transform.localRotation = (Quaternion)rotation * Quaternion.LookRotation(direction, Vector3.forward) * Quaternion.AngleAxis(mousePitch, Vector3.right);
		radius = pqs.GetSurfaceHeight(realPos) + altitude;
		realPos *= radius;
		base.transform.localPosition = realPos;
		cameraTransform.localPosition = Vector3.zero;
		cameraTransform.localRotation = Quaternion.identity;
	}

	private void UpdateTransformOverview()
	{
		realPos = rotation * Vector3d.forward;
		base.transform.localRotation = (Quaternion)rotation * Quaternion.LookRotation(direction, Vector3.forward);
		radius = pqs.GetSurfaceHeight(realPos) + altitude;
		realPos *= radius;
		base.transform.localPosition = realPos;
		Vector3 localPosition = Quaternion.AngleAxis(elevationAngle, Vector3.right) * new Vector3(0f, 0f, 0f - zoom);
		cameraTransform.localPosition = localPosition;
		cameraTransform.localRotation = Quaternion.AngleAxis(elevationAngle, Vector3.right);
	}
}
