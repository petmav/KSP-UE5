using System;
using FinePrint;

public class ProtoWaypointInfo : IConfigNode
{
	private enum Type
	{
		Custom,
		Survey,
		Null
	}

	public string name;

	public Guid navigationId;

	private Type waypointType;

	public ProtoWaypointInfo(ProtoWaypointInfo pWI)
	{
		name = pWI.name;
		navigationId = pWI.navigationId;
		waypointType = pWI.waypointType;
	}

	public ProtoWaypointInfo(Waypoint wp)
	{
		if (wp != null)
		{
			waypointType = Type.Custom;
			navigationId = wp.navigationId;
		}
		else
		{
			waypointType = Type.Null;
			navigationId = Guid.Empty;
		}
	}

	public ProtoWaypointInfo()
	{
		waypointType = Type.Null;
	}

	public ProtoWaypointInfo(ConfigNode node)
	{
		waypointType = Type.Null;
		Load(node);
	}

	public void Load(ConfigNode node)
	{
		if (node.HasValue("type"))
		{
			waypointType = (Type)Enum.Parse(typeof(Type), node.GetValue("type"));
		}
		if (node.HasValue("navigationID"))
		{
			navigationId = new Guid(node.GetValue("navigationID"));
		}
	}

	public void Save(ConfigNode node)
	{
		if (waypointType != Type.Null)
		{
			node.AddValue("type", waypointType.ToString());
			node.AddValue("navigationID", navigationId.ToString());
		}
	}

	public Waypoint FindWaypoint()
	{
		switch (waypointType)
		{
		default:
			return null;
		case Type.Custom:
			return WaypointManager.FindWaypoint(navigationId);
		case Type.Survey:
			return WaypointManager.FindWaypoint(navigationId);
		case Type.Null:
			return null;
		}
	}
}
