using System;
using UnityEngine;

public class CometVFXController : MonoBehaviour
{
	[SerializeField]
	private Transform dustTailFarEffect;

	[SerializeField]
	private ParticleSystem dustTailFarParticleSystem;

	[SerializeField]
	private Transform ionTailFarEffect;

	[SerializeField]
	private ParticleSystem ionTailFarParticleSystem;

	[SerializeField]
	internal SphereVolume comaEffect;

	[SerializeField]
	private float comaConstrastAtZero = 3.2f;

	[SerializeField]
	private float comaConstrastAtOne = 2f;

	[SerializeField]
	private float comaTextureScaleAtZero = 50f;

	[SerializeField]
	private float comaTextureScaleAtOne = 100f;

	[SerializeField]
	private float comaVisibilityAtZero;

	[SerializeField]
	private float comaVisibilityAtOne = 1f;

	internal CometParticleController ionController;

	internal CometParticleController dustController;

	private bool vfxInitialized;

	public ScaledMovement hostSM;

	public Vessel hostVessel;

	public CometVessel cometVessel;

	public float[] comaTimeWarpMovement;

	[SerializeField]
	private Part cometPart;

	[SerializeField]
	private Vector3d directionFromSun;

	[SerializeField]
	private bool vesselDead;

	private Vector3 comaMovement;

	internal void Initialize()
	{
		if (hostSM == null)
		{
			hostSM = base.gameObject.GetComponentInParent<ScaledMovement>();
		}
		if (hostVessel == null && hostSM != null)
		{
			hostVessel = hostSM.vessel;
		}
		if (cometVessel == null && hostVessel != null)
		{
			cometVessel = hostVessel.gameObject.GetComponent<CometVessel>();
			if (cometVessel != null)
			{
				cometVessel.cometVFX = this;
			}
		}
		if (hostSM != null && hostSM.firstUpdate && cometVessel != null && cometVessel.vfxRatio > 0f && hostVessel != null && hostVessel.orbitDriver != null && hostVessel.orbitDriver.Ready)
		{
			SpawnVFXObjects();
		}
	}

	private void DeSpawnVFX()
	{
		if (dustTailFarEffect != null)
		{
			UnityEngine.Object.Destroy(dustTailFarEffect.gameObject);
			dustTailFarParticleSystem = null;
		}
		if (ionTailFarEffect != null)
		{
			UnityEngine.Object.Destroy(ionTailFarEffect.gameObject);
			ionTailFarParticleSystem = null;
		}
		if (comaEffect != null)
		{
			UnityEngine.Object.Destroy(comaEffect.gameObject);
		}
		vfxInitialized = false;
	}

	private void SpawnVFXObjects()
	{
		DefineDustTailParticleFX();
		DefineIONTailParticleFX();
		DefineComaFX();
		vfxInitialized = true;
	}

	private void DefineDustTailParticleFX()
	{
		if (dustTailFarEffect == null)
		{
			dustTailFarEffect = UnityEngine.Object.Instantiate(CometManager.Instance.dustTailParticleFarEffect);
			dustTailFarEffect.SetParent(hostSM.transform);
			dustTailFarEffect.localPosition = Vector3.zero;
			dustTailFarEffect.rotation = Quaternion.identity;
			dustController = dustTailFarEffect.GetComponent<CometParticleController>();
			dustController.comet = cometVessel;
			dustController.tailWidthComaRatio = (float)cometVessel.tailMaxWidthRatio;
			dustController.emitterColor = CometManager.GenerateWeightedDustTailColor(cometVessel);
			dustTailFarEffect.gameObject.layer = 10;
			dustTailFarParticleSystem = dustTailFarEffect.GetComponent<ParticleSystem>();
		}
	}

	private void DefineIONTailParticleFX()
	{
		if (ionTailFarEffect == null)
		{
			ionTailFarEffect = UnityEngine.Object.Instantiate(CometManager.Instance.ionTailParticleFarEffect);
			ionTailFarEffect.SetParent(hostSM.transform);
			ionTailFarEffect.localPosition = Vector3.zero;
			ionTailFarEffect.rotation = Quaternion.identity;
			ionController = ionTailFarEffect.GetComponent<CometParticleController>();
			ionController.comet = cometVessel;
			ionController.tailWidthComaRatio = (float)cometVessel.ionTailMaxWidthRatio;
			ionController.emitterColor = CometManager.GenerateWeightedIonTailColor(cometVessel);
			ionTailFarEffect.gameObject.layer = 10;
			ionTailFarParticleSystem = ionTailFarEffect.GetComponent<ParticleSystem>();
		}
	}

	private void DefineComaFX()
	{
		if (comaEffect == null)
		{
			Transform transform = UnityEngine.Object.Instantiate(CometManager.Instance.comaNearEffect);
			comaEffect = transform.gameObject.GetComponent<SphereVolume>();
			if (comaEffect != null)
			{
				comaEffect.transform.SetParent(hostSM.transform);
				comaEffect.transform.localPosition = Vector3.zero;
				comaEffect.radius = cometVessel.ComaRadiusScaledSpace;
				UpdateComaFX();
				comaEffect.UpdateVolume();
			}
		}
	}

	private void OnVesselWasLoaded(Vessel v)
	{
		if (!(hostVessel != null) || !(v == hostVessel) || !(cometPart == null))
		{
			return;
		}
		int num = 0;
		while (true)
		{
			if (num < hostVessel.Parts.Count)
			{
				if (hostVessel.Parts[num].HasModuleImplementing<ModuleComet>())
				{
					break;
				}
				num++;
				continue;
			}
			return;
		}
		cometPart = hostVessel.Parts[num];
		cometPart.highlightType = Part.HighlightType.Disabled;
	}

	private void OnCometVesselChanged(Vessel oldVessel, Vessel newVessel, CometVessel oldCometVessel, CometVessel newCometVessel)
	{
		if (hostVessel != null && oldVessel.persistentId == hostVessel.persistentId && oldCometVessel == cometVessel)
		{
			hostVessel = newVessel;
			cometVessel = newCometVessel;
			hostSM = hostVessel.mapObject.GetComponentInParent<ScaledMovement>();
			base.transform.SetParent(hostSM.transform);
			base.transform.localPosition = Vector3.zero;
			base.transform.rotation = Quaternion.identity;
			if (dustTailFarEffect != null)
			{
				dustTailFarEffect.SetParent(hostSM.transform);
				dustTailFarEffect.localPosition = Vector3.zero;
				dustTailFarEffect.rotation = Quaternion.identity;
			}
			if (ionTailFarEffect != null)
			{
				ionTailFarEffect.SetParent(hostSM.transform);
				ionTailFarEffect.localPosition = Vector3.zero;
				ionTailFarEffect.rotation = Quaternion.identity;
			}
			if (comaEffect != null)
			{
				comaEffect.transform.SetParent(hostSM.transform);
				comaEffect.transform.localPosition = Vector3.zero;
			}
		}
	}

	private void OnPartWillDie(Part p)
	{
		if (cometVessel != null && p.persistentId == cometVessel.cometPartId)
		{
			vesselDead = true;
			if (dustTailFarParticleSystem != null)
			{
				ParticleSystem.EmissionModule emission = dustTailFarParticleSystem.emission;
				emission.enabled = false;
			}
			if (ionTailFarParticleSystem != null)
			{
				ParticleSystem.EmissionModule emission2 = ionTailFarParticleSystem.emission;
				emission2.enabled = false;
			}
			if (comaEffect != null)
			{
				UnityEngine.Object.Destroy(comaEffect.gameObject);
			}
		}
	}

	private void UpdateComaFX()
	{
		if (cometVessel != null && comaEffect != null)
		{
			comaEffect.visibility = Mathf.Lerp(comaVisibilityAtZero, comaVisibilityAtOne, cometVessel.vfxRatio) * cometVessel.atmosphereVFXRatio;
			comaEffect.radius = cometVessel.ComaRadiusScaledSpace;
			comaEffect.Contrast = Mathf.Lerp(comaConstrastAtZero, comaConstrastAtOne, cometVessel.vfxRatio);
			comaEffect.textureScale = Mathf.Lerp(comaTextureScaleAtZero, comaTextureScaleAtOne, cometVessel.vfxRatio);
			int num = Math.Min(comaTimeWarpMovement.Length - 1, TimeWarp.CurrentRateIndex);
			int num2 = Mathf.Max(0, num - 1);
			comaMovement = new Vector3(comaTimeWarpMovement[num2], comaTimeWarpMovement[num2], comaTimeWarpMovement[num2]);
			comaMovement.x = Mathf.Lerp(comaMovement.x, comaTimeWarpMovement[num], TimeWarp.CurrentRate / TimeWarp.fetch.tgt_rate) * (float)directionFromSun.normalized.x;
			comaMovement.y = Mathf.Lerp(comaMovement.y, comaTimeWarpMovement[num], TimeWarp.CurrentRate / TimeWarp.fetch.tgt_rate) * (float)directionFromSun.normalized.y;
			comaMovement.z = Mathf.Lerp(comaMovement.z, comaTimeWarpMovement[num], TimeWarp.CurrentRate / TimeWarp.fetch.tgt_rate) * (float)directionFromSun.normalized.z;
			comaEffect.textureMovement = comaMovement;
			if (!comaEffect.gameObject.activeInHierarchy)
			{
				comaEffect.gameObject.SetActive(value: true);
			}
		}
	}

	private void Start()
	{
		if (comaTimeWarpMovement == null || comaTimeWarpMovement.Length < 1)
		{
			comaTimeWarpMovement = new float[8] { 0.01f, 0.05f, 0.1f, 0.2f, 0.3f, 0.4f, 0.6f, 0.75f };
		}
		comaMovement = Vector3.one * comaTimeWarpMovement[0];
		Initialize();
		GameEvents.onVesselLoaded.Add(OnVesselWasLoaded);
		GameEvents.onCometVesselChanged.Add(OnCometVesselChanged);
		GameEvents.onPartWillDie.Add(OnPartWillDie);
	}

	private void OnDestroy()
	{
		GameEvents.onVesselLoaded.Remove(OnVesselWasLoaded);
		GameEvents.onCometVesselChanged.Remove(OnCometVesselChanged);
		GameEvents.onPartWillDie.Remove(OnPartWillDie);
	}

	private void LateUpdate()
	{
		if (vesselDead)
		{
			return;
		}
		if (!vfxInitialized && cometVessel != null && cometVessel.vfxRatio > 0f)
		{
			Initialize();
		}
		if (!vfxInitialized)
		{
			return;
		}
		if (vfxInitialized && cometVessel != null && cometVessel.vfxRatio <= 0f && cometVessel.vfxRatioSet)
		{
			DeSpawnVFX();
			return;
		}
		UpdateComaFX();
		if (!(hostVessel == null) && !(hostSM == null) && hostVessel.orbitDriver.Ready)
		{
			directionFromSun = ((Vector3d)hostSM.transform.position - ScaledSpace.LocalToScaledSpace(SunFlare.Instance.sun.position)).normalized;
			Vector3d xzy = hostVessel.orbit.GetFrameVel().xzy;
			Vector3d vector3d = Vector3d.Cross(directionFromSun, xzy);
			if (dustTailFarEffect != null)
			{
				dustTailFarEffect.rotation = Quaternion.LookRotation(directionFromSun, vector3d);
			}
			if (ionTailFarEffect != null)
			{
				ionTailFarEffect.rotation = Quaternion.LookRotation(directionFromSun, vector3d);
			}
		}
	}
}
