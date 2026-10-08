using UnityEngine;

public class ReentryFXParticleTrail : MonoBehaviour
{
	private Vessel vessel;

	public ParticleSystem pSys;

	public AerodynamicsFX fxLogic;

	public Vector3 velocity;

	public float effectSize = 1f;

	public float effectIntensity;

	private Rigidbody _vesselRigidbody;

	private void Start()
	{
		vessel = GetComponent<Vessel>();
		if (pSys == null)
		{
			pSys = (Object.Instantiate(Resources.Load("Effects/fx_reentryTrail")) as GameObject).GetComponent<ParticleSystem>();
		}
	}

	private void FixedUpdate()
	{
		if ((bool)vessel)
		{
			velocity = vessel.srf_velocity;
			pSys.transform.position = vessel.CoM + vessel.GetComponentCached(ref _vesselRigidbody).velocity * Time.fixedDeltaTime;
		}
		if ((bool)fxLogic)
		{
			velocity = fxLogic.velocity * (float)fxLogic.airSpeed;
			effectIntensity = fxLogic.FxScalar;
		}
		pSys.transform.forward = -velocity.normalized;
		ParticleSystem.MainModule main = pSys.main;
		main.startDelay = velocity.magnitude;
	}
}
