using System;
using System.Collections.Generic;
using UnityEngine;

public class InternalSpaceOverlay : MonoBehaviour
{
	private Callback onDismiss;

	private Camera ivaCam;

	private Transform ivaCamTrf;

	private Camera extCam;

	private Transform extCamTrf;

	private Vessel vessel;

	public static InternalSpaceOverlay Create(Vessel v, Callback onDismiss)
	{
		InternalSpaceOverlay internalSpaceOverlay = new GameObject("InternalSpaceOverlay Host").AddComponent<InternalSpaceOverlay>();
		internalSpaceOverlay.vessel = v;
		internalSpaceOverlay.onDismiss = onDismiss;
		internalSpaceOverlay.Setup();
		return internalSpaceOverlay;
	}

	private void Setup()
	{
		GameEvents.onVesselWasModified.Add(OnVesselModified);
		GameEvents.OnCameraChange.Add(OnCameraChange);
		GameEvents.onCrewTransferred.Add(onCrewTransferred);
		ivaCam = base.gameObject.AddComponent<Camera>();
		ivaCam.cullingMask = 1114112;
		ivaCam.clearFlags = CameraClearFlags.Depth;
		ivaCam.depth = 2f;
		ivaCam.allowHDR = false;
		ivaCamTrf = ivaCam.transform;
		ivaCamTrf.parent = InternalSpace.Instance.transform;
		extCam = FlightCamera.fetch.cameras[0];
		extCamTrf = extCam.transform;
		ivaCam.fieldOfView = extCam.fieldOfView;
		vessel.SetActiveInternalSpaces(new HashSet<Part>(vessel.parts));
	}

	protected void OnCameraChange(CameraManager.CameraMode data)
	{
		Dismiss();
	}

	protected void onCrewTransferred(GameEvents.HostedFromToAction<ProtoCrewMember, Part> data)
	{
		Dismiss();
	}

	private void OnVesselModified(Vessel v)
	{
		Dismiss();
	}

	public void LateUpdate()
	{
		if (Input.GetKeyUp(KeyCode.Escape))
		{
			Dismiss();
		}
		ivaCamTrf.position = InternalSpace.WorldToInternal(extCamTrf.position);
		ivaCamTrf.rotation = InternalSpace.WorldToInternal(extCamTrf.rotation);
		ivaCam.fieldOfView = extCam.fieldOfView;
	}

	public void Dismiss()
	{
		try
		{
			if ((bool)base.gameObject)
			{
				UnityEngine.Object.Destroy(base.gameObject);
			}
		}
		catch (Exception)
		{
			Debug.LogError("Error dismissing IVA, get_gameobject or the destroy threw.");
		}
	}

	private void OnDestroy()
	{
		if (onDismiss != null)
		{
			onDismiss();
		}
		GameEvents.onVesselWasModified.Remove(OnVesselModified);
		GameEvents.OnCameraChange.Remove(OnCameraChange);
		GameEvents.onCrewTransferred.Remove(onCrewTransferred);
	}
}
