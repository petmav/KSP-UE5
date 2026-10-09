using UnityEngine;

public class Controller : MonoBehaviour
{
	private Transform _camera;

	private Transform _LookAtTarget;

	private bool _isControllingTorus = true;

	public void Awake()
	{
		SpaceNavigator.SetTranslationSensitivity(1f);
		SpaceNavigator.SetRotationSensitivity(1f);
		_camera = Camera.main.transform;
		_LookAtTarget = GameObject.FindGameObjectWithTag("Torus look at target").transform;
	}

	public void Update()
	{
		if (Input.GetKeyDown(KeyCode.Space))
		{
			_isControllingTorus = !_isControllingTorus;
		}
		if (_isControllingTorus)
		{
			base.transform.Translate(_camera.transform.TransformDirection(SpaceNavigator.Translation), Space.World);
			base.transform.rotation = SpaceNavigator.RotationInLocalCoordSys(_camera) * base.transform.rotation;
			_camera.Translate(SpaceNavigator.Translation, Space.Self);
			_camera.LookAt(_LookAtTarget);
		}
		else
		{
			_camera.RotateAround(base.transform.position, Vector3.up, SpaceNavigator.Rotation.Yaw() * 57.29578f);
			_camera.RotateAround(base.transform.position, _camera.transform.right, SpaceNavigator.Rotation.Pitch() * 57.29578f);
		}
	}
}
