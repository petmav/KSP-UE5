using System.Collections;
using UnityEngine;

[RequireComponent(typeof(Renderer))]
public class SunShaderController : MonoBehaviour
{
	public bool usePlanetariumTime;

	public float speedFactor = 1f;

	public float frequency0 = 4f;

	public AnimationCurve curve0 = new AnimationCurve(new Keyframe(0f, 0f), new Keyframe(0.5f, 1f), new Keyframe(1f, 0f));

	public float frequency1 = 3f;

	public AnimationCurve curve1 = new AnimationCurve(new Keyframe(0f, 0f), new Keyframe(0.5f, 1f), new Keyframe(1f, 0f));

	public float frequency2 = 2f;

	public AnimationCurve curve2 = new AnimationCurve(new Keyframe(0f, 0f), new Keyframe(0.5f, 1f), new Keyframe(1f, 0f));

	public float frequency3 = 1f;

	public AnimationCurve curve3 = new AnimationCurve(new Keyframe(0f, 0f), new Keyframe(0.5f, 1f), new Keyframe(1f, 0f));

	private Renderer r;

	private float time;

	private float rate0;

	private float rate1;

	private float rate2;

	private float rate3;

	private Texture2D rampMap;

	private static int shaderPropertyOffset0;

	private static int shaderPropertyOffset1;

	private static int shaderPropertyOffset2;

	private static int shaderPropertyOffset3;

	private IEnumerator Start()
	{
		shaderPropertyOffset0 = Shader.PropertyToID("_Offset0");
		shaderPropertyOffset1 = Shader.PropertyToID("_Offset1");
		shaderPropertyOffset2 = Shader.PropertyToID("_Offset2");
		shaderPropertyOffset3 = Shader.PropertyToID("_Offset3");
		r = GetComponent<Renderer>();
		rate0 = 1f / frequency0;
		rate1 = 1f / frequency1;
		rate2 = 1f / frequency2;
		rate3 = 1f / frequency3;
		yield return null;
		UpdateRampMap();
	}

	private void OnDestroy()
	{
		if (rampMap != null)
		{
			Object.Destroy(rampMap);
		}
	}

	[ContextMenu("Update Ramp Map")]
	private void UpdateRampMap()
	{
		if (rampMap != null)
		{
			Object.Destroy(rampMap);
		}
		rampMap = new Texture2D(256, 3, TextureFormat.RGBA32, mipChain: false);
		rampMap.filterMode = FilterMode.Bilinear;
		float num = 0.00390625f;
		Color color = default(Color);
		for (int i = 0; i < 256; i++)
		{
			color.r = curve0.Evaluate((float)i * num);
			color.g = curve1.Evaluate((float)i * num);
			color.b = curve2.Evaluate((float)i * num);
			color.a = curve3.Evaluate((float)i * num);
			for (int j = 0; j < 5; j++)
			{
				rampMap.SetPixel(i, j, color);
			}
		}
		rampMap.Apply(updateMipmaps: false, makeNoLongerReadable: true);
		r.material.SetTexture("_RampMap", rampMap);
	}

	private void Update()
	{
		if (usePlanetariumTime && Planetarium.fetch != null)
		{
			time = (float)Planetarium.GetUniversalTime() * speedFactor;
		}
		else
		{
			time = Time.realtimeSinceStartup * speedFactor;
		}
		r.material.SetFloat(shaderPropertyOffset0, rate0 * time);
		r.material.SetFloat(shaderPropertyOffset1, rate1 * time);
		r.material.SetFloat(shaderPropertyOffset2, rate2 * time);
		r.material.SetFloat(shaderPropertyOffset3, rate3 * time);
	}
}
