using UnityEngine;

public class FlyAround : MonoBehaviour
{
	public bool HorizonLock = true;

	public void Update()
	{
		base.transform.Translate(SpaceNavigator.Translation, Space.Self);
		if (HorizonLock)
		{
			base.transform.Rotate(Vector3.up, SpaceNavigator.Rotation.Yaw() * 57.29578f, Space.World);
			base.transform.Rotate(Vector3.right, SpaceNavigator.Rotation.Pitch() * 57.29578f, Space.Self);
		}
		else
		{
			base.transform.Rotate(SpaceNavigator.Rotation.eulerAngles, Space.Self);
		}
	}
}
