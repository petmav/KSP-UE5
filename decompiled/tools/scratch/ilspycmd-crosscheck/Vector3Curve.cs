using System;
using UnityEngine;

[Serializable]
public class Vector3Curve
{
	[SerializeField]
	private AnimationCurve x;

	[SerializeField]
	private AnimationCurve y;

	[SerializeField]
	private AnimationCurve z;

	public float minTime { get; private set; }

	public float maxTime { get; private set; }

	public Vector3Curve()
	{
		x = new AnimationCurve();
		y = new AnimationCurve();
		z = new AnimationCurve();
		minTime = float.MaxValue;
		maxTime = float.MinValue;
	}

	public void Add(float time, Vector3 value)
	{
		x.AddKey(time, value.x);
		y.AddKey(time, value.y);
		z.AddKey(time, value.z);
		minTime = Mathf.Min(minTime, time);
		maxTime = Mathf.Max(maxTime, time);
	}

	public void Add(float time, Color value)
	{
		x.AddKey(time, value.r);
		y.AddKey(time, value.g);
		z.AddKey(time, value.b);
		minTime = Mathf.Min(minTime, time);
		maxTime = Mathf.Max(maxTime, time);
	}

	public Vector3 EvaluateVector(float time)
	{
		return new Vector3(x.Evaluate(time), y.Evaluate(time), z.Evaluate(time));
	}

	public Color EvaluateColor(float time)
	{
		return new Color(x.Evaluate(time), y.Evaluate(time), z.Evaluate(time));
	}
}
