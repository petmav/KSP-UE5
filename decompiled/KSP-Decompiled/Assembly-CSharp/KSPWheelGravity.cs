using UnityEngine;

public class KSPWheelGravity
{
	private Vector3 gravityValue = Vector3.down * 9.80665f;

	private Vector3 gravityUp;

	private bool gravityCached;

	private float gravityMagnitude;

	public Vector3 Value
	{
		get
		{
			return gravityValue;
		}
		set
		{
			if (gravityValue != value)
			{
				gravityValue = value;
				Refresh();
			}
		}
	}

	public float Magnitude
	{
		get
		{
			if (!gravityCached)
			{
				Refresh();
			}
			return gravityMagnitude;
		}
	}

	public Vector3 Up
	{
		get
		{
			if (!gravityCached)
			{
				Refresh();
			}
			return gravityUp;
		}
	}

	public void Refresh()
	{
		gravityMagnitude = gravityValue.magnitude;
		gravityUp = (-gravityValue).normalized;
		gravityCached = true;
	}
}
