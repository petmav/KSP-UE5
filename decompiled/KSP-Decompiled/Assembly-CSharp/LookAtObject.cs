using UnityEngine;

public class LookAtObject : MonoBehaviour
{
	public Transform target;

	public Vector3 fwdAxis = Vector3.forward;

	public KFSMUpdateMode updateMode = KFSMUpdateMode.UPDATE;

	private Transform trf;

	private void Awake()
	{
		trf = base.transform;
	}

	private void FixedUpdate()
	{
		if (updateMode == KFSMUpdateMode.FIXEDUPDATE)
		{
			onUpdate();
		}
	}

	private void Update()
	{
		if (updateMode == KFSMUpdateMode.UPDATE)
		{
			onUpdate();
		}
	}

	private void LateUpdate()
	{
		if (updateMode == KFSMUpdateMode.LATEUPDATE)
		{
			onUpdate();
		}
	}

	private void onUpdate()
	{
		trf.rotation = Quaternion.LookRotation(Quaternion.FromToRotation(trf.rotation * fwdAxis, target.position - trf.position) * trf.forward, target.up);
	}
}
