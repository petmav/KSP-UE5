using System;
using System.Collections;
using System.Collections.Generic;
using Expansions;
using KSP.UI;
using KSP.UI.Screens;
using UnityEngine;
using UnityEngine.UI;

public class MapViewFiltering : MonoBehaviour
{
	[Serializable]
	public class FilterButton
	{
		public TrackingStationObjectButton button;

		public VesselTypeFilter filterValue;

		public int typeCount;

		public bool showCount;

		private MapViewFiltering Host;

		public void Init(MapViewFiltering host)
		{
			Host = host;
		}

		public void SetSolo()
		{
			if ((filterValue & VesselTypeFilter.DeployedScienceController) != 0 && !ExpansionsLoader.IsExpansionInstalled("Serenity"))
			{
				filterValue ^= VesselTypeFilter.DeployedScienceController;
			}
			if (vesselTypeFilter != filterValue)
			{
				Host.setFilter(filterValue);
			}
			else
			{
				Host.setFilter(VesselTypeFilter.All);
			}
		}
	}

	[Flags]
	public enum VesselTypeFilter
	{
		None = 0,
		Debris = 1,
		Unknown = 2,
		SpaceObjects = 4,
		Probes = 8,
		Rovers = 0x10,
		Landers = 0x20,
		Ships = 0x40,
		Stations = 0x80,
		Bases = 0x100,
		EVAs = 0x200,
		Flags = 0x400,
		Plane = 0x800,
		Relay = 0x1000,
		Site = 0x2000,
		DeployedScienceController = 0x4000,
		All = -1
	}

	public static MapViewFiltering Instance;

	public UIPanelTransitionToggle panel;

	public FilterButton[] FilterButtons;

	public Button showHideButton;

	[SerializeField]
	private RectTransform backgroundImageRect;

	[SerializeField]
	private RectTransform commNetRect;

	public static VesselTypeFilter vesselTypeFilter;

	private bool started;

	public bool showObjectCounts = true;

	private Dictionary<VesselTypeFilter, FilterButton> vCounts;

	private void Awake()
	{
		if (Instance != null)
		{
			UnityEngine.Object.Destroy(this);
		}
		else
		{
			Instance = this;
		}
		GameEvents.onVesselCreate.Add(onVesselCountChanged);
		GameEvents.onVesselWillDestroy.Add(onVesselCountChanged);
		GameEvents.onVesselTerminated.Add(onVesselCountChanged);
		GameEvents.onVesselRecovered.Add(onVesselCountChanged);
		GameEvents.onKnowledgeChanged.Add(onKnowledgeChanged);
		GameEvents.onInputLocksModified.Add(OnInputLocksModified);
		GameEvents.onVesselRename.Add(onVesselRename);
		if (showHideButton != null)
		{
			showHideButton.onClick.AddListener(ToggleFiltersPanel);
		}
	}

	private void OnDestroy()
	{
		GameEvents.onVesselCreate.Remove(onVesselCountChanged);
		GameEvents.onVesselWillDestroy.Remove(onVesselCountChanged);
		GameEvents.onVesselTerminated.Remove(onVesselCountChanged);
		GameEvents.onVesselRecovered.Remove(onVesselCountChanged);
		GameEvents.onKnowledgeChanged.Remove(onKnowledgeChanged);
		GameEvents.onInputLocksModified.Remove(OnInputLocksModified);
		GameEvents.onVesselRename.Remove(onVesselRename);
		if (showHideButton != null)
		{
			showHideButton.onClick.RemoveListener(ToggleFiltersPanel);
		}
		if (Instance != null && Instance == this)
		{
			Instance = null;
		}
	}

	public static int GetFilterState()
	{
		return (int)vesselTypeFilter;
	}

	public static void LoadFilterState(int filterState)
	{
		if (((uint)filterState & 0x4000u) != 0 && !ExpansionsLoader.IsExpansionInstalled("Serenity"))
		{
			vesselTypeFilter = (VesselTypeFilter)(filterState ^ 0x4000);
		}
		else
		{
			vesselTypeFilter = (VesselTypeFilter)filterState;
		}
	}

	private void Start()
	{
		if (!started)
		{
			Init();
		}
	}

	private void Init()
	{
		if (!started)
		{
			vCounts = new Dictionary<VesselTypeFilter, FilterButton>();
			int i = 0;
			for (int num = FilterButtons.Length; i < num; i++)
			{
				FilterButton filterButton = FilterButtons[i];
				if (filterButton.filterValue == VesselTypeFilter.DeployedScienceController && !ExpansionsLoader.IsExpansionInstalled("Serenity"))
				{
					filterButton.button.gameObject.SetActive(value: false);
					if (backgroundImageRect != null)
					{
						backgroundImageRect.sizeDelta = new Vector2(1815f, backgroundImageRect.rect.height);
					}
					if (commNetRect != null)
					{
						commNetRect.anchoredPosition = new Vector2(660f, commNetRect.anchoredPosition.y);
					}
				}
				else
				{
					filterButton.button.OnClickLeft.AddListener(updateFilterFromButtons);
					filterButton.button.OnClickRight.AddListener(filterButton.SetSolo);
					filterButton.Init(this);
					vCounts.Add(filterButton.filterValue, filterButton);
				}
			}
			RefreshCounts();
		}
		if (vesselTypeFilter == VesselTypeFilter.None)
		{
			vesselTypeFilter = ~(VesselTypeFilter.Debris | VesselTypeFilter.Flags);
		}
		updateButtonsToFilter(vesselTypeFilter);
		started = true;
	}

	private void CountVessels()
	{
		if (!showObjectCounts || vCounts == null || vCounts.Count == 0 || FlightGlobals.Vessels == null)
		{
			return;
		}
		foreach (KeyValuePair<VesselTypeFilter, FilterButton> vCount in vCounts)
		{
			vCount.Value.typeCount = 0;
		}
		for (int i = 0; i < FlightGlobals.Vessels.Count; i++)
		{
			Vessel vessel = FlightGlobals.Vessels[i];
			VesselType vesselType = VesselType.Unknown;
			if (vessel.DiscoveryInfo.HaveKnowledgeAbout(DiscoveryLevels.Appearance))
			{
				switch (vessel.vesselType)
				{
				case VesselType.Debris:
					vCounts[VesselTypeFilter.Debris].typeCount++;
					continue;
				case VesselType.SpaceObject:
					vCounts[VesselTypeFilter.SpaceObjects].typeCount++;
					continue;
				case VesselType.Probe:
					vCounts[VesselTypeFilter.Probes].typeCount++;
					continue;
				case VesselType.Relay:
					vCounts[VesselTypeFilter.Relay].typeCount++;
					continue;
				case VesselType.Rover:
					vCounts[VesselTypeFilter.Rovers].typeCount++;
					continue;
				case VesselType.Lander:
					vCounts[VesselTypeFilter.Landers].typeCount++;
					continue;
				case VesselType.Ship:
					vCounts[VesselTypeFilter.Ships].typeCount++;
					continue;
				case VesselType.Plane:
					vCounts[VesselTypeFilter.Plane].typeCount++;
					continue;
				case VesselType.Station:
					vCounts[VesselTypeFilter.Stations].typeCount++;
					continue;
				case VesselType.Base:
					vCounts[VesselTypeFilter.Bases].typeCount++;
					continue;
				case VesselType.EVA:
					vCounts[VesselTypeFilter.EVAs].typeCount++;
					continue;
				case VesselType.Flag:
					vCounts[VesselTypeFilter.Flags].typeCount++;
					continue;
				case VesselType.DeployedScienceController:
					if (ExpansionsLoader.IsExpansionInstalled("Serenity"))
					{
						vCounts[VesselTypeFilter.DeployedScienceController].typeCount++;
					}
					continue;
				case VesselType.DeployedSciencePart:
					continue;
				}
			}
			vCounts[VesselTypeFilter.Unknown].typeCount++;
		}
		vCounts[VesselTypeFilter.Site].typeCount = MapView.fetch.siteNodes.Count;
	}

	internal void RefreshCounts()
	{
		CountVessels();
		UpdateVesselCounts();
	}

	private IEnumerator RefreshInOneFrame()
	{
		yield return null;
		RefreshCounts();
	}

	private void onVesselCountChanged(Vessel v)
	{
		StartCoroutine(RefreshInOneFrame());
	}

	private void onVesselCountChanged(ProtoVessel protoVessel)
	{
		StartCoroutine(RefreshInOneFrame());
	}

	private void onVesselCountChanged(ProtoVessel protoVessel, bool quick)
	{
		StartCoroutine(RefreshInOneFrame());
	}

	private void UpdateVesselCounts()
	{
		int i = 0;
		for (int num = FilterButtons.Length; i < num; i++)
		{
			FilterButtons[i].button.SetCount(FilterButtons[i].typeCount);
			FilterButtons[i].button.ShowCount(showObjectCounts);
		}
	}

	private void onKnowledgeChanged(GameEvents.HostedFromToAction<IDiscoverable, DiscoveryLevels> dsc)
	{
		if (dsc.host is Vessel)
		{
			RefreshCounts();
		}
	}

	private void onVesselRename(GameEvents.HostedFromToAction<Vessel, string> nChg)
	{
		RefreshCounts();
	}

	private void OnInputLocksModified(GameEvents.FromToAction<ControlTypes, ControlTypes> action)
	{
		if (!InputLockManager.IsLocking(ControlTypes.TRACKINGSTATION_UI, action) && !InputLockManager.IsLocking(ControlTypes.MAP_UI, action))
		{
			if (InputLockManager.IsUnlocking(ControlTypes.TRACKINGSTATION_UI, action) || InputLockManager.IsUnlocking(ControlTypes.MAP_UI, action))
			{
				int i = 0;
				for (int num = FilterButtons.Length; i < num; i++)
				{
					FilterButtons[i].button.Unlock();
				}
			}
		}
		else
		{
			int j = 0;
			for (int num2 = FilterButtons.Length; j < num2; j++)
			{
				FilterButtons[j].button.Lock();
			}
		}
	}

	private VesselTypeFilter GetFilterFromButtonStates()
	{
		VesselTypeFilter vesselTypeFilter = VesselTypeFilter.None;
		int i = 0;
		for (int num = FilterButtons.Length; i < num; i++)
		{
			FilterButton filterButton = FilterButtons[i];
			if (filterButton.button.isActiveAndEnabled)
			{
				vesselTypeFilter = (VesselTypeFilter)((int)vesselTypeFilter | ((filterButton.button.state ? 1 : 0) * (int)filterButton.filterValue));
			}
		}
		return vesselTypeFilter;
	}

	public void updateFilterFromButtons()
	{
		vesselTypeFilter = GetFilterFromButtonStates();
		GameEvents.OnMapViewFiltersModified.Fire(vesselTypeFilter);
	}

	public void updateButtonsToFilter(VesselTypeFilter filter)
	{
		if ((filter & VesselTypeFilter.DeployedScienceController) != 0 && !ExpansionsLoader.IsExpansionInstalled("Serenity"))
		{
			filter ^= VesselTypeFilter.DeployedScienceController;
		}
		int i = 0;
		for (int num = FilterButtons.Length; i < num; i++)
		{
			FilterButton filterButton = FilterButtons[i];
			if (filterButton.button.isActiveAndEnabled)
			{
				filterButton.button.SetState((filter & filterButton.filterValue) != 0);
			}
		}
	}

	public void setFilter(VesselTypeFilter filter)
	{
		if ((filter & VesselTypeFilter.DeployedScienceController) != 0 && !ExpansionsLoader.IsExpansionInstalled("Serenity"))
		{
			filter ^= VesselTypeFilter.DeployedScienceController;
		}
		vesselTypeFilter = filter;
		updateButtonsToFilter(filter);
		GameEvents.OnMapViewFiltersModified.Fire(filter);
	}

	private bool checkAgainstFilter(Vessel v)
	{
		VesselType vesselType = VesselType.Unknown;
		if (v.DiscoveryInfo.HaveKnowledgeAbout(DiscoveryLevels.Appearance))
		{
			vesselType = v.vesselType;
		}
		Vessel activeVessel = FlightGlobals.ActiveVessel;
		if (activeVessel != null)
		{
			if (activeVessel == v)
			{
				return true;
			}
			if (FlightGlobals.fetch.VesselTarget != null && FlightGlobals.fetch.VesselTarget.GetVessel() == v)
			{
				return true;
			}
		}
		switch (vesselType)
		{
		case VesselType.Debris:
			return (vesselTypeFilter & VesselTypeFilter.Debris) != 0;
		case VesselType.SpaceObject:
			return (vesselTypeFilter & VesselTypeFilter.SpaceObjects) != 0;
		default:
			return (vesselTypeFilter & VesselTypeFilter.Unknown) != 0;
		case VesselType.Probe:
			return (vesselTypeFilter & VesselTypeFilter.Probes) != 0;
		case VesselType.Relay:
			return (vesselTypeFilter & VesselTypeFilter.Relay) != 0;
		case VesselType.Rover:
			return (vesselTypeFilter & VesselTypeFilter.Rovers) != 0;
		case VesselType.Lander:
			return (vesselTypeFilter & VesselTypeFilter.Landers) != 0;
		case VesselType.Ship:
			return (vesselTypeFilter & VesselTypeFilter.Ships) != 0;
		case VesselType.Plane:
			return (vesselTypeFilter & VesselTypeFilter.Plane) != 0;
		case VesselType.Station:
			return (vesselTypeFilter & VesselTypeFilter.Stations) != 0;
		case VesselType.Base:
			return (vesselTypeFilter & VesselTypeFilter.Bases) != 0;
		case VesselType.EVA:
			return (vesselTypeFilter & VesselTypeFilter.EVAs) != 0;
		case VesselType.Flag:
			return (vesselTypeFilter & VesselTypeFilter.Flags) != 0;
		case VesselType.DeployedScienceController:
			return (vesselTypeFilter & VesselTypeFilter.DeployedScienceController) != 0;
		case VesselType.DeployedSciencePart:
			return false;
		}
	}

	public void ToggleFiltersPanel()
	{
		if (panel.expanded)
		{
			panel.Collapse();
		}
		else if (panel.collapsed)
		{
			panel.Expand();
		}
	}

	public static void SetFilter(VesselTypeFilter filter)
	{
		if (Instance != null)
		{
			Instance.setFilter(filter);
		}
	}

	public static bool CheckAgainstFilter(Vessel v)
	{
		if (!(Instance == null) && !(v == null))
		{
			return Instance.checkAgainstFilter(v);
		}
		return true;
	}

	public static void Initialize()
	{
		if (Instance != null)
		{
			Instance.Init();
		}
	}
}
