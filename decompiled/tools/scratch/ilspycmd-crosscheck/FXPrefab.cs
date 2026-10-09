using UnityEngine;

[RequireComponent(typeof(ParticleSystem))]
public class FXPrefab : MonoBehaviour
{
	private ParticleSystem particleSystem;

	private static Vector3 currentKrakensbaneFixedDelta;

	private void Awake()
	{
		particleSystem = GetComponent<ParticleSystem>();
	}

	private void OnEnable()
	{
		FloatingOrigin.RegisterParticleSystem(particleSystem);
	}

	private void OnDisable()
	{
		FloatingOrigin.UnregisterParticleSystem(particleSystem);
	}
}
