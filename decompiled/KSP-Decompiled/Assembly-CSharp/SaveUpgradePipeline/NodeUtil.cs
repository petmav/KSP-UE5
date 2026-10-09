using System;
using System.Collections.Generic;
using UnityEngine;

namespace SaveUpgradePipeline
{
	public static class NodeUtil
	{
		public static Version GetCfgVersion(ConfigNode n, LoadContext loadContext)
		{
			if (loadContext == LoadContext.SFS)
			{
				return new Version(n.GetNode("GAME").GetValue("version"));
			}
			return new Version(n.GetValue("version"));
		}

		public static string GetPartNodeNameValue(ConfigNode node, LoadContext loadContext)
		{
			switch (loadContext)
			{
			default:
				return string.Empty;
			case LoadContext.Craft:
				return node.GetValue("part");
			case LoadContext.SFS:
				return node.GetValue("name");
			}
		}

		public static ConfigNode[] GetModuleNodesNamed(ConfigNode node, string moduleName)
		{
			List<ConfigNode> list = new List<ConfigNode>();
			ConfigNode[] nodes = node.GetNodes("MODULE");
			for (int i = 0; i < nodes.Length; i++)
			{
				if (nodes[i].HasValue("name") && nodes[i].GetValue("name") == moduleName)
				{
					list.Add(nodes[i]);
				}
			}
			return list.ToArray();
		}

		public static void SetPartNodeNameValue(ConfigNode node, LoadContext loadContext, string newNameValue)
		{
			switch (loadContext)
			{
			case LoadContext.Craft:
				node.SetValue("part", newNameValue);
				break;
			case LoadContext.SFS:
				node.SetValue("name", newNameValue);
				break;
			}
		}

		public static string GetPartPosition(ConfigNode node, LoadContext loadContext)
		{
			switch (loadContext)
			{
			default:
				return string.Empty;
			case LoadContext.Craft:
				return node.GetValue("pos");
			case LoadContext.SFS:
				return node.GetValue("position");
			}
		}

		public static void SetPartPosition(ConfigNode node, LoadContext loadContext, Vector3 newPos)
		{
			switch (loadContext)
			{
			case LoadContext.Craft:
				node.SetValue("pos", KSPUtil.WriteVector(newPos));
				break;
			case LoadContext.SFS:
				node.SetValue("position", KSPUtil.WriteVector(newPos));
				break;
			}
		}

		public static string GetPartRotation(ConfigNode node, LoadContext loadContext)
		{
			switch (loadContext)
			{
			default:
				return string.Empty;
			case LoadContext.Craft:
				return node.GetValue("rot");
			case LoadContext.SFS:
				return node.GetValue("rotation");
			}
		}

		public static void SetPartRotation(ConfigNode node, LoadContext loadContext, Quaternion newRot)
		{
			switch (loadContext)
			{
			case LoadContext.Craft:
				node.SetValue("rot", newRot);
				break;
			case LoadContext.SFS:
				node.SetValue("rotation", newRot);
				break;
			}
		}
	}
}
