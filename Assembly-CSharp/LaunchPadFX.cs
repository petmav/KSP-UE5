using UnityEngine;

public class LaunchPadFX : MonoBehaviour
{
	[SerializeField]
	protected ParticleSystem[] ps;

	[SerializeField]
	protected Material smokeParticleMaterial;

	[Range(0f, 1f)]
	[SerializeField]
	protected float fxScale;

	[SerializeField]
	private float maxFX = 5f;

	private float totalFX;

	private void Start()
	{
		int num = ps.Length;
		while (num-- > 0)
		{
			FloatingOrigin.RegisterParticleSystem(ps[num]);
		}
	}

	private void OnDestroy()
	{
		int num = ps.Length;
		while (num-- > 0)
		{
			FloatingOrigin.UnregisterParticleSystem(ps[num]);
		}
	}

	public void AddFX(float fx)
	{
		totalFX = Mathf.Clamp(totalFX + fx, 0f, maxFX);
		fxScale = totalFX / maxFX;
	}

	private void LateUpdate()
	{
		int num = ps.Length;
		while (num-- > 0)
		{
			if (fxScale > 0f)
			{
				ParticleSystem.MainModule main = ps[num].main;
				if (!ps[num].isPlaying)
				{
					ps[num].Play();
				}
				if (!ps[num].emission.enabled)
				{
					ParticleSystem.EmissionModule emission = ps[num].emission;
					emission.enabled = true;
				}
				main.startColor = main.startColor.color.A(Mathf.Lerp(0f, 0.5f, fxScale));
			}
			if (fxScale == 0f)
			{
				ParticleSystem.EmissionModule emission2 = ps[num].emission;
				emission2.enabled = false;
			}
		}
		totalFX = 0f;
		fxScale = 0f;
	}
}
