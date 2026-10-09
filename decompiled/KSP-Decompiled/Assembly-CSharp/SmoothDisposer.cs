using Smooth.Dispose;
using UnityEngine;

public class SmoothDisposer : MonoBehaviour
{
	private static SmoothDisposer _instance;

	private void Awake()
	{
		if ((bool)_instance)
		{
			Debug.LogWarning("Only one " + GetType().Name + " should exist at a time, instantiated by the " + typeof(DisposalQueue).Name + " class.");
			Object.Destroy(this);
		}
		else
		{
			_instance = this;
			Object.DontDestroyOnLoad(this);
		}
	}

	private void LateUpdate()
	{
		DisposalQueue.Pulse();
	}
}
