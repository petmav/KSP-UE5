using UnityEngine;

public class EditorMarker : MonoBehaviour
{
	public GameObject posMarkerObject;

	public GameObject dirMarkerObject;

	public Part rootPart;

	public void Update()
	{
		Vector3 position = UpdatePosition();
		if ((bool)posMarkerObject)
		{
			posMarkerObject.transform.position = position;
		}
		if ((bool)dirMarkerObject)
		{
			dirMarkerObject.transform.position = position;
		}
	}

	protected virtual Vector3 UpdatePosition()
	{
		return Vector3.zero;
	}

	protected virtual Vector3 UpdateDirection()
	{
		return Vector3.zero;
	}
}
