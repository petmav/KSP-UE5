using System;
using UnityEngine;

public class SkySphereControl : MonoBehaviour
{
	private static int shaderPropertyColor2;

	private static int shaderPropertyDayNightBlend;

	private static int shaderPropertySpaceBlend;

	public float skyFadeStart = 0.3f;

	public float atmosphereLimit = 15000f;

	public Color dayTimeSpaceColorShift;

	public Sun sunRef;

	public Camera tgt;

	public double sunSrfAngle;

	public double sunCamAngle;

	private QuaternionD initRot;

	private Renderer _renderer;

	private void Start()
	{
		initRot = base.transform.rotation;
		shaderPropertyDayNightBlend = Shader.PropertyToID("_dayNightBlend");
		shaderPropertyColor2 = Shader.PropertyToID("_Color2");
		shaderPropertySpaceBlend = Shader.PropertyToID("_spaceBlend");
	}

	private void Update()
	{
		sunSrfAngle = Math.Acos(Vector3d.Dot(-sunRef.sunDirection, FlightGlobals.getUpAxis(ScaledSpace.ScaledToLocalSpace(tgt.transform.position)))) * 57.295780181884766;
		sunCamAngle = Math.Acos(Vector3d.Dot(-sunRef.sunDirection, tgt.transform.forward)) * 57.295780181884766;
		sunSrfAngle = Math.Max(0.0, Math.Min(1.0, (sunSrfAngle - 80.0) / 20.0));
		if (double.IsNaN(sunSrfAngle))
		{
			sunSrfAngle = 0.0;
		}
		sunCamAngle = Math.Max(0.0, Math.Min(1.0, (sunCamAngle - (double)tgt.fieldOfView * 0.5) / 20.0));
		this.GetComponentCached(ref _renderer).material.SetFloat(shaderPropertyDayNightBlend, (float)sunSrfAngle);
		this.GetComponentCached(ref _renderer).material.SetColor(shaderPropertyColor2, MapView.MapIsEnabled ? Color.white : Color.Lerp(dayTimeSpaceColorShift, Color.white, (float)sunCamAngle));
		this.GetComponentCached(ref _renderer).material.SetFloat(shaderPropertySpaceBlend, Mathf.InverseLerp(atmosphereLimit * skyFadeStart, atmosphereLimit, (float)FlightGlobals.getAltitudeAtPos(ScaledSpace.ScaledToLocalSpace(tgt.transform.position))));
	}

	private void FixedUpdate()
	{
		base.transform.rotation = Planetarium.Rotation * initRot;
	}
}
