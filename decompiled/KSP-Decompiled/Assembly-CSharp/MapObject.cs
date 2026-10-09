using Expansions.Missions;
using KSP.UI.Screens.Mapview;
using UnityEngine;

public class MapObject : MonoBehaviour
{
	public enum ObjectType
	{
		Null,
		Generic,
		CelestialBody,
		Vessel,
		ManeuverNode,
		Periapsis,
		Apoapsis,
		AscendingNode,
		DescendingNode,
		ApproachIntersect,
		CelestialBodyAtUT,
		PatchTransition,
		MENode,
		Site
	}

	public Transform trf;

	public Transform tgtRef;

	public Vessel vessel;

	public CelestialBody celestialBody;

	public ManeuverNode maneuverNode;

	public MENode missionsNode;

	public LaunchSite launchSite;

	public ObjectType type;

	public Orbit orbit;

	public IDiscoverable Discoverable;

	public MapNode uiNode;

	public string DisplayName;

	internal Vessel vesselRef;

	public static MapObject Create(string name, string displayName, Orbit orbit, ManeuverNode node)
	{
		MapObject mapObject = new GameObject(name).AddComponent<MapObject>();
		mapObject.orbit = orbit;
		mapObject.type = ObjectType.ManeuverNode;
		mapObject.maneuverNode = node;
		mapObject.DisplayName = displayName;
		return mapObject;
	}

	public static MapObject Create(string name, string displayName, Transform tgtRef, Orbit orbit, ObjectType type)
	{
		MapObject mapObject = new GameObject(name).AddComponent<MapObject>();
		if (tgtRef != null)
		{
			mapObject.tgtRef = tgtRef;
		}
		mapObject.orbit = orbit;
		mapObject.type = type;
		mapObject.DisplayName = displayName;
		return mapObject;
	}

	public static MapObject Create(string name, string displayName, Orbit orbit, ObjectType type)
	{
		MapObject mapObject = new GameObject(name).AddComponent<MapObject>();
		mapObject.orbit = orbit;
		mapObject.type = type;
		mapObject.DisplayName = displayName;
		return mapObject;
	}

	private void Awake()
	{
		trf = base.transform;
		trf.parent = ScaledSpace.Instance.transform;
		OnAwake();
	}

	protected virtual void OnAwake()
	{
	}

	private void Start()
	{
		if (tgtRef != null)
		{
			vessel = tgtRef.GetComponent<Vessel>();
			if (vessel != null)
			{
				type = ObjectType.Vessel;
				Discoverable = vessel;
				orbit = vessel.orbit;
			}
			celestialBody = tgtRef.GetComponent<CelestialBody>();
			if (celestialBody != null)
			{
				type = ObjectType.CelestialBody;
				Discoverable = celestialBody;
				orbit = celestialBody.orbit;
			}
		}
		ScaledSpace.AddScaledSpaceObject(this);
		OnStart();
	}

	protected virtual void OnStart()
	{
	}

	private void LateUpdate()
	{
		OnLateUpdate();
	}

	protected virtual void OnLateUpdate()
	{
	}

	public string GetName()
	{
		switch (type)
		{
		default:
			return OnGetName();
		case ObjectType.CelestialBody:
			return celestialBody.DiscoveryInfo.name.Value;
		case ObjectType.Vessel:
			return vessel.DiscoveryInfo.name.Value;
		case ObjectType.Null:
		case ObjectType.ManeuverNode:
			return DisplayName;
		}
	}

	public string GetDisplayName()
	{
		ObjectType objectType = type;
		if (objectType == ObjectType.CelestialBody)
		{
			return celestialBody.DiscoveryInfo.displayName.Value;
		}
		return GetName();
	}

	protected virtual string OnGetName()
	{
		return base.name;
	}

	public CelestialBody GetReferenceBody()
	{
		if ((bool)celestialBody)
		{
			return celestialBody;
		}
		if ((bool)vessel)
		{
			return vessel.orbit.referenceBody;
		}
		if (maneuverNode != null)
		{
			return maneuverNode.patch.referenceBody;
		}
		return orbit.referenceBody;
	}

	public void Terminate()
	{
		OnWillDestroy();
		if (uiNode != null)
		{
			uiNode.Terminate();
		}
		Object.Destroy(base.gameObject);
	}

	private void OnDestroy()
	{
		ScaledSpace.RemoveScaledSpaceObject(this);
	}

	protected virtual void OnWillDestroy()
	{
	}
}
