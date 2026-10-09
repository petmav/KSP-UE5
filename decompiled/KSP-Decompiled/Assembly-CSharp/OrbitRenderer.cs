using UnityEngine;
using UnityEngine.EventSystems;

public class OrbitRenderer : OrbitRendererBase
{
	protected bool orbitDisplayUnlocked;

	protected override void Start()
	{
		base.Start();
		if ((bool)vessel)
		{
			base.objName = vessel.GetDisplayName();
			discoveryInfo = vessel.DiscoveryInfo;
			orbitDisplayUnlocked = GameVariables.Instance.GetOrbitDisplayMode(ScenarioUpgradeableFacilities.GetFacilityLevel(SpaceCenterFacility.TrackingStation)) >= GameVariables.OrbitDisplayMode.AllOrbits;
			SetColor(new Color(0.71f, 0.71f, 0.71f, 1f));
		}
		else if ((bool)celestialBody)
		{
			base.objName = base.name;
			discoveryInfo = celestialBody.DiscoveryInfo;
			orbitDisplayUnlocked = GameVariables.Instance.GetOrbitDisplayMode(ScenarioUpgradeableFacilities.GetFacilityLevel(SpaceCenterFacility.TrackingStation)) >= GameVariables.OrbitDisplayMode.CelestialBodyOrbits;
			if (PSystemManager.OrbitRendererDataCache.ContainsKey(celestialBody))
			{
				OrbitRendererData orbitRendererData = PSystemManager.OrbitRendererDataCache[celestialBody];
				orbitColor = orbitRendererData.orbitColor;
				nodeColor = orbitRendererData.nodeColor;
				lowerCamVsSmaRatio = orbitRendererData.lowerCamVsSmaRatio;
				upperCamVsSmaRatio = orbitRendererData.upperCamVsSmaRatio;
			}
		}
		else if ((bool)meNode)
		{
			base.objName = base.name;
			discoveryInfo = new DiscoveryInfo(meNode);
			orbitDisplayUnlocked = true;
		}
		else
		{
			base.objName = base.name;
			orbitDisplayUnlocked = true;
		}
		GameEvents.onGameSceneLoadRequested.Add(OnSceneChange);
	}

	protected override void OnDestroy()
	{
		base.OnDestroy();
		GameEvents.onGameSceneLoadRequested.Remove(OnSceneChange);
	}

	private void OnSceneChange(GameScenes scene)
	{
		Object.Destroy(this);
	}

	protected override void DrawOrbit(DrawMode mode)
	{
		if (!(MapView.fetch == null) && MapView.MapIsEnabled && !(PlanetariumCamera.fetch.target == null) && base.OrbitLine != null)
		{
			bool flag = IsDebris();
			bool flag2 = MapViewFiltering.CheckAgainstFilter(vessel);
			base.OrbitLine.active = GetActive(mode, flag, flag2);
			if ((!flag || base.mouseOver) && flag2)
			{
				base.DrawOrbit(mode);
			}
		}
	}

	private bool GetActive(DrawMode mode, bool isDebris, bool isUnfiltered)
	{
		return mode != 0 && lineOpacity > 0f && (!isDebris || base.mouseOver) && orbitDisplayUnlocked && discoveryInfo.HaveKnowledgeAbout(DiscoveryLevels.StateVectors) && isUnfiltered;
	}

	private bool IsDebris()
	{
		if ((bool)vessel && vessel.vesselType == VesselType.Debris)
		{
			return !isFocused;
		}
		return false;
	}

	protected override Color GetNodeColour()
	{
		if (!isFocused)
		{
			return nodeColor.A(lineOpacity);
		}
		return XKCDColors.ElectricLime;
	}

	protected override Color GetOrbitColour()
	{
		if (!isFocused)
		{
			return orbitColor.A(lineOpacity);
		}
		return XKCDColors.ElectricLime;
	}

	protected override bool CanDrawAnyIcons()
	{
		if (!MapView.MapIsEnabled)
		{
			return false;
		}
		return base.CanDrawAnyIcons();
	}

	public override bool OrbitCast(Vector3 screenPos, out OrbitCastHit hitInfo, float orbitPixelWidth = 10f)
	{
		hitInfo = default(OrbitCastHit);
		hitInfo.or = this;
		hitInfo.driver = driver;
		if (EventSystem.current.IsPointerOverGameObject())
		{
			return false;
		}
		return base.OrbitCast(screenPos, out hitInfo, orbitPixelWidth);
	}
}
