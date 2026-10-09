using System;
using System.Collections.Generic;
using CameraKeyFrameEvents;
using UnityEngine;

public class CutsceneCamera : MonoBehaviour
{
	[Serializable]
	public class CameraKeyFrame
	{
		public enum Easing
		{
			Linear,
			SinLerp,
			CosLerp,
			Hermite,
			Berp
		}

		public Transform refTrf;

		public float duration = 1f;

		public int index;

		[HideInInspector]
		public CameraKeyFrame nextFrm;

		public Easing EaseToNextFrameMode = Easing.Hermite;

		public List<CameraKeyFrameEvent> frameEvents;

		public Vector3 EvalPos(float normT)
		{
			if (nextFrm != null)
			{
				switch (EaseToNextFrameMode)
				{
				default:
					return Vector3.Lerp(refTrf.position, nextFrm.refTrf.position, normT);
				case Easing.SinLerp:
					return Vector3.Lerp(refTrf.position, nextFrm.refTrf.position, Mathfx.Sinerp(0f, 1f, normT));
				case Easing.CosLerp:
					return Vector3.Lerp(refTrf.position, nextFrm.refTrf.position, Mathfx.Coserp(0f, 1f, normT));
				case Easing.Hermite:
					return Vector3.Lerp(refTrf.position, nextFrm.refTrf.position, Mathfx.Hermite(0f, 1f, normT));
				case Easing.Berp:
					return Vector3.Lerp(refTrf.position, nextFrm.refTrf.position, Mathfx.Berp(0f, 1f, normT));
				}
			}
			return refTrf.position;
		}

		public Quaternion EvalRot(float normT)
		{
			if (nextFrm != null)
			{
				switch (EaseToNextFrameMode)
				{
				default:
					return Quaternion.Lerp(refTrf.rotation, nextFrm.refTrf.rotation, normT);
				case Easing.SinLerp:
					return Quaternion.Lerp(refTrf.rotation, nextFrm.refTrf.rotation, Mathfx.Sinerp(0f, 1f, normT));
				case Easing.CosLerp:
					return Quaternion.Lerp(refTrf.rotation, nextFrm.refTrf.rotation, Mathfx.Coserp(0f, 1f, normT));
				case Easing.Hermite:
					return Quaternion.Lerp(refTrf.rotation, nextFrm.refTrf.rotation, Mathfx.Hermite(0f, 1f, normT));
				case Easing.Berp:
					return Quaternion.Lerp(refTrf.rotation, nextFrm.refTrf.rotation, Mathfx.Berp(0f, 1f, normT));
				}
			}
			return refTrf.rotation;
		}

		public void EvalEvents(float normT)
		{
			for (int i = 0; i < frameEvents.Count; i++)
			{
				CameraKeyFrameEvent cameraKeyFrameEvent = frameEvents[i];
				if (normT >= cameraKeyFrameEvent.timeIntoFrame && !cameraKeyFrameEvent.done)
				{
					cameraKeyFrameEvent.RunEvent();
					cameraKeyFrameEvent.done = true;
				}
			}
		}
	}

	public float totalDuration = 65f;

	public int i;

	public List<CameraKeyFrame> keyFrames;

	private float T;

	private float normT;

	private float lastFrameT;

	private CameraKeyFrame currentFrame;

	private int cFrameIndex;

	private float speedFactor = 1f;

	private void Start()
	{
		T = Time.realtimeSinceStartup;
		lastFrameT = T;
		cFrameIndex = 0;
		if (keyFrames.Count > 0)
		{
			currentFrame = keyFrames[cFrameIndex];
			float num = 0f;
			for (int i = 0; i < keyFrames.Count - 1; i++)
			{
				keyFrames[i].nextFrm = keyFrames[i + 1];
				num += keyFrames[i].duration;
			}
			speedFactor = totalDuration / num;
		}
	}

	private void Update()
	{
		if (currentFrame == null)
		{
			return;
		}
		T = Time.realtimeSinceStartup;
		normT = (T - lastFrameT) / (currentFrame.duration * speedFactor);
		currentFrame.EvalEvents(normT);
		if (normT >= 1f)
		{
			if (cFrameIndex + 1 >= keyFrames.Count)
			{
				currentFrame = null;
				return;
			}
			cFrameIndex++;
			currentFrame = keyFrames[cFrameIndex];
			lastFrameT = T;
			normT = 1f - normT;
		}
		base.transform.position = currentFrame.EvalPos(normT);
		base.transform.rotation = currentFrame.EvalRot(normT);
	}

	[ContextMenu("Add Frame Here")]
	public void CreateKeyFrame()
	{
		Transform transform = new GameObject("CameraKeyFrame #" + i).transform;
		transform.position = base.transform.position;
		transform.rotation = base.transform.rotation;
		i++;
		CameraKeyFrame cameraKeyFrame = new CameraKeyFrame();
		cameraKeyFrame.refTrf = transform;
		keyFrames.Add(cameraKeyFrame);
	}

	private int kfCompare(CameraKeyFrame a, CameraKeyFrame b)
	{
		return a.index.CompareTo(b.index);
	}

	[ContextMenu("Sort KeyFrames")]
	public void SortKeyFrames()
	{
		keyFrames.Sort(kfCompare);
		for (int i = 0; i < keyFrames.Count; i++)
		{
			keyFrames[i].refTrf.name = "CameraKeyFrame #" + i;
		}
	}

	[ContextMenu("Go To Frame i")]
	public void GoToFrame()
	{
		base.transform.position = keyFrames[i].EvalPos(0f);
		base.transform.rotation = keyFrames[i].EvalRot(0f);
	}
}
