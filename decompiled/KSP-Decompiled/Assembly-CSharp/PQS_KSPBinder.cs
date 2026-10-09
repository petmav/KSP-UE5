using UnityEngine;

public class PQS_KSPBinder : MonoBehaviour
{
	[SerializeField]
	private PQS_GameBindings pqsBindings;

	private void Awake()
	{
		pqsBindings = PQS.GameBindings;
		pqsBindings.GetSettingsReady += pqsBindings_GetSettingsReady;
		pqsBindings.GetPlanetForceShaderModel20 += pqsBindings_GetPlanetForceShaderModel20;
		pqsBindings.GetUsePlanetScatter += pqsBindings_GetUsePlanetScatter;
		pqsBindings.GetPlanetScatterFactor += pqsBindings_GetPlanetScatterFactor;
		pqsBindings.GetLoadedSceneIsGame += pqsBindings_GetLoadedSceneIsGame;
		pqsBindings.GetPresetListCompatible += pqsBindings_GetPresetListCompatible;
		pqsBindings.OnPQSCityLoaded += OnPQSCityLoaded;
		pqsBindings.OnPQSCityUnloaded += OnPQSCityUnloaded;
		pqsBindings.OnGetPQSCityLoadRange += GetPQSCityLoadRange;
		pqsBindings.OnGetPOIRange += OnGetPOIRange;
	}

	private bool pqsBindings_GetPresetListCompatible(string versionstring)
	{
		return KSPUtil.CheckVersion(versionstring, PQSCache.lastCompatibleMajor, PQSCache.lastCompatibleMinor, PQSCache.lastCompatibleRev) == VersionCompareResult.COMPATIBLE;
	}

	private bool pqsBindings_GetLoadedSceneIsGame()
	{
		return HighLogic.LoadedSceneIsGame;
	}

	private float pqsBindings_GetPlanetScatterFactor()
	{
		return GameSettings.PLANET_SCATTER_FACTOR;
	}

	private bool pqsBindings_GetUsePlanetScatter()
	{
		return GameSettings.PLANET_SCATTER;
	}

	private bool pqsBindings_GetPlanetForceShaderModel20()
	{
		return GameSettings.UNSUPPORTED_LEGACY_SHADER_TERRAIN;
	}

	private bool pqsBindings_GetSettingsReady()
	{
		return GameSettings.Ready;
	}

	protected void OnPQSCityLoaded(PQSSurfaceObject city)
	{
		CelestialBody cBForPQS = GetCBForPQS(city.sphere);
		if (cBForPQS != null)
		{
			GameEvents.OnPQSCityLoaded.Fire(cBForPQS, city.gameObject.name);
		}
	}

	protected void OnPQSCityUnloaded(PQSSurfaceObject city)
	{
		CelestialBody cBForPQS = GetCBForPQS(city.sphere);
		if (cBForPQS != null)
		{
			GameEvents.OnPQSCityUnloaded.Fire(cBForPQS, city.gameObject.name);
		}
	}

	protected double GetPQSCityLoadRange(PQSSurfaceObject city)
	{
		CelestialBody cBForPQS = GetCBForPQS(city.sphere);
		VesselRanges vesselRanges = ((PhysicsGlobals.Instance != null) ? new VesselRanges(PhysicsGlobals.Instance.VesselRangesDefault) : new VesselRanges());
		return (cBForPQS == null || !cBForPQS.atmosphere) ? vesselRanges.subOrbital.load : vesselRanges.flying.load;
	}

	protected double OnGetPOIRange(PQSSurfaceObject surfaceObject)
	{
		CelestialBody cBForPQS = GetCBForPQS(surfaceObject.sphere);
		VesselRanges vesselRanges = ((PhysicsGlobals.Instance != null) ? new VesselRanges(PhysicsGlobals.Instance.VesselRangesDefault) : new VesselRanges());
		return (cBForPQS == null || !cBForPQS.atmosphere) ? vesselRanges.subOrbital.load : vesselRanges.flying.load;
	}

	protected CelestialBody GetCBForPQS(PQS pqs)
	{
		return pqs.gameObject.GetComponentUpwards<CelestialBody>();
	}

	private void OnDestroy()
	{
		if (pqsBindings != null)
		{
			pqsBindings.GetSettingsReady -= pqsBindings_GetSettingsReady;
			pqsBindings.GetPlanetForceShaderModel20 -= pqsBindings_GetPlanetForceShaderModel20;
			pqsBindings.GetUsePlanetScatter -= pqsBindings_GetUsePlanetScatter;
			pqsBindings.GetPlanetScatterFactor -= pqsBindings_GetPlanetScatterFactor;
			pqsBindings.GetLoadedSceneIsGame -= pqsBindings_GetLoadedSceneIsGame;
			pqsBindings.GetPresetListCompatible -= pqsBindings_GetPresetListCompatible;
			pqsBindings.OnPQSCityLoaded -= OnPQSCityLoaded;
			pqsBindings.OnPQSCityUnloaded -= OnPQSCityUnloaded;
			pqsBindings.OnGetPQSCityLoadRange -= GetPQSCityLoadRange;
		}
	}
}
