using UnityEngine;

public class ModuleSurfaceFX : PartModule
{
	public enum SurfaceType
	{
		None,
		Terrain,
		Water,
		Launchpad
	}

	[KSPField]
	public int thrustProviderModuleIndex = -1;

	[KSPField]
	public float fxMax = 1f;

	[KSPField]
	public float maxDistance = 50f;

	[KSPField]
	public float falloff = 2f;

	[KSPField]
	public string thrustTransformName = "";

	private GameObject terrainPrefab;

	private GameObject waterPrefab;

	private IThrustProvider engineModule;

	private Transform trf;

	private RaycastHit hitInfo;

	private SurfaceType hit;

	private float fxScale = 1f;

	private float distance;

	private float scaledDistance;

	private float h0;

	private float dH;

	private bool raycastHit;

	public Vector3 rDir;

	public Vector3 Vsrf;

	public Vector3 point;

	public Vector3 normal;

	public float ScaledFX;

	private SurfaceFX srfFX;

	private SurfaceFX srfFXnext;

	private LaunchPadFX padFX;

	private void Start()
	{
		terrainPrefab = AssetBase.GetPrefab("SurfaceFX");
		waterPrefab = AssetBase.GetPrefab("WaterFX");
		if (!string.IsNullOrEmpty(thrustTransformName))
		{
			trf = base.part.FindModelTransform(thrustTransformName);
		}
		else
		{
			trf = base.part.partTransform;
		}
		if (thrustProviderModuleIndex != -1)
		{
			engineModule = base.part.Modules[thrustProviderModuleIndex] as IThrustProvider;
		}
		if (engineModule == null)
		{
			Debug.LogError("[ModuleSrfFX]: No IThrustProvider module found at index " + thrustProviderModuleIndex + "!", base.gameObject);
		}
	}

	private void Update()
	{
		if (!HighLogic.LoadedSceneIsFlight || !GameSettings.SURFACE_FX)
		{
			return;
		}
		if (engineModule != null)
		{
			fxScale = engineModule.GetCurrentThrust() / engineModule.GetMaxThrust() * fxMax;
		}
		else
		{
			fxScale = 0f;
		}
		if (fxScale > 0f && (!base.part.vessel.mainBody.ocean || FlightGlobals.getAltitudeAtPos((Vector3d)trf.position, base.part.vessel.mainBody) > 0.0))
		{
			raycastHit = Physics.Raycast(trf.position, trf.forward, out hitInfo, maxDistance, 1073774592);
			float num;
			float altitudeAtPos;
			if (raycastHit && FlightGlobals.GetSqrAltitude(hitInfo.point, base.part.vessel.mainBody) >= 0.0)
			{
				if (hitInfo.collider.CompareTag("LaunchpadFX"))
				{
					hit = SurfaceType.Launchpad;
				}
				else if (hitInfo.collider.CompareTag("Wheel_Piston_Collider"))
				{
					hit = SurfaceType.None;
					Vsrf = Vector3.zero;
					ScaledFX = 0f;
				}
				else
				{
					hit = SurfaceType.Terrain;
				}
				point = hitInfo.point;
				normal = hitInfo.normal;
				distance = hitInfo.distance;
			}
			else if (base.part.vessel.mainBody.ocean && (num = Vector3.Dot(trf.forward, -base.vessel.upAxis)) > 0f && (altitudeAtPos = FlightGlobals.getAltitudeAtPos(trf.position, base.vessel.mainBody)) < maxDistance && altitudeAtPos > 0f)
			{
				normal = base.vessel.upAxis;
				distance = altitudeAtPos / num;
				point = trf.position + trf.forward * distance;
				hit = SurfaceType.Water;
			}
			else
			{
				hit = SurfaceType.None;
			}
			scaledDistance = Mathf.Pow(1f - distance / maxDistance, falloff);
			ScaledFX = fxScale * scaledDistance;
			rDir = point - trf.position;
			Vsrf = Vector3.ProjectOnPlane(rDir, normal).normalized * fxScale;
		}
		else
		{
			hit = SurfaceType.None;
			Vsrf = Vector3.zero;
			ScaledFX = 0f;
		}
		UpdateSrfFX(hitInfo);
	}

	private void OnDrawGizmos()
	{
		if (hit != 0)
		{
			Gizmos.color = XKCDColors.Red;
			Gizmos.DrawWireSphere(point, 2f);
			Gizmos.DrawSphere(point, 0.25f);
		}
	}

	private void UpdateSrfFX(RaycastHit hitInfo)
	{
		switch (hit)
		{
		default:
			srfFXnext = null;
			padFX = null;
			break;
		case SurfaceType.Terrain:
			srfFXnext = GetSurfaceFX(srfFX, terrainPrefab, point, normal);
			padFX = null;
			break;
		case SurfaceType.Water:
			srfFXnext = GetSurfaceFX(srfFX, waterPrefab, point, normal);
			padFX = null;
			break;
		case SurfaceType.Launchpad:
			srfFXnext = null;
			if (padFX == null)
			{
				padFX = hitInfo.collider.gameObject.GetComponent<LaunchPadFX>();
			}
			break;
		}
		if (srfFXnext != srfFX)
		{
			if (srfFX != null)
			{
				srfFX.RemoveSource(this);
			}
			if (srfFXnext != null)
			{
				srfFXnext.AddSource(this);
			}
			srfFX = srfFXnext;
		}
		if (padFX != null)
		{
			padFX.AddFX(ScaledFX);
		}
	}

	private SurfaceFX GetSurfaceFX(SurfaceFX sFX, GameObject prefabToSpawn, Vector3 wPos, Vector3 wNormal)
	{
		SurfaceFX surfaceFX = SurfaceFX.FindNearestFX(this, wPos);
		if (surfaceFX != null)
		{
			if (sFX == null)
			{
				return surfaceFX;
			}
			if (sFX.ScaledFX < surfaceFX.ScaledFX)
			{
				return surfaceFX;
			}
			return sFX;
		}
		if (sFX != null && sFX.leadSource == this && sFX.prefab == prefabToSpawn)
		{
			return sFX;
		}
		surfaceFX = Object.Instantiate(prefabToSpawn).GetComponent<SurfaceFX>();
		surfaceFX.prefab = prefabToSpawn;
		return surfaceFX;
	}

	private void OnDestroy()
	{
		if (srfFX != null)
		{
			srfFX.RemoveSource(this);
		}
	}
}
