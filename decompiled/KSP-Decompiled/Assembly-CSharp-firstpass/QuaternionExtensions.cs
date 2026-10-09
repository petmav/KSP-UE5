using UnityEngine;

public static class QuaternionExtensions
{
	public static Quaternion Inverse(this Quaternion q)
	{
		return Quaternion.Inverse(q);
	}

	public static Quaternion QuaternionFromMatrix(Matrix4x4 m)
	{
		return Quaternion.LookRotation(m.GetColumn(2), m.GetColumn(1));
	}

	public static Quaternion RotateInSpecifiedCoordinateSystem(Quaternion rotation, Transform referenceCoordSys)
	{
		return referenceCoordSys.rotation * rotation * referenceCoordSys.rotation.Inverse();
	}

	public static float Pitch(this Quaternion q)
	{
		return Mathf.Atan2(2f * (q.y * q.z + q.w * q.x), q.w * q.w - q.x * q.x - q.y * q.y + q.z * q.z);
	}

	public static float Yaw(this Quaternion q)
	{
		return Mathf.Asin(-2f * (q.x * q.z - q.w * q.y));
	}

	public static float Roll(this Quaternion q)
	{
		return Mathf.Atan2(2f * (q.x * q.y + q.w * q.z), q.w * q.w + q.x * q.x - q.y * q.y - q.z * q.z);
	}
}
