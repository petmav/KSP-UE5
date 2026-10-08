using UnityEngine;

public class TrackRigObject : MonoBehaviour
{
	public enum TrackMode
	{
		FixedUpdate,
		Update,
		LateUpdate
	}

	public Transform target;

	public bool keepInitialOffset;

	private bool initialized;

	public TrackMode trackingMode = TrackMode.Update;

	private Transform trf;

	private Vector3 pOff;

	private Quaternion rOff;

	private void Awake()
	{
		Initialize();
	}

	[ContextMenu("Initialize")]
	private void Initialize()
	{
		if (!(target == null))
		{
			trf = base.transform;
			pOff = trf.position - target.position;
			rOff = Quaternion.Inverse(target.rotation) * trf.rotation;
			initialized = true;
		}
	}

	private void FixedUpdate()
	{
		if (trackingMode == TrackMode.FixedUpdate)
		{
			Track();
		}
	}

	private void Update()
	{
		if (trackingMode == TrackMode.Update)
		{
			Track();
		}
	}

	private void LateUpdate()
	{
		if (trackingMode == TrackMode.LateUpdate)
		{
			Track();
		}
	}

	private void Track()
	{
		if (!initialized)
		{
			Initialize();
		}
		if (!(target == null))
		{
			if (keepInitialOffset)
			{
				trf.rotation = target.rotation * rOff;
				trf.position = target.position + trf.rotation * pOff;
			}
			else
			{
				trf.position = target.position;
				trf.rotation = target.rotation;
			}
		}
	}
}
