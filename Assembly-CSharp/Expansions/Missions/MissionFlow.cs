using System.Collections.Generic;
using Expansions.Missions.Flow;

namespace Expansions.Missions
{
	public class MissionFlow : IConfigNode
	{
		internal Dictionary<MENode, MENodePathInfo> NodePaths;

		internal Dictionary<MENode, MENodePathInfo> NodeReversePaths;

		internal MEFlowBlock missionBlock;

		private Mission mission;

		public MissionFlow(Mission mission)
		{
			this.mission = mission;
			NodePaths = new Dictionary<MENode, MENodePathInfo>();
			NodeReversePaths = new Dictionary<MENode, MENodePathInfo>();
			missionBlock = new MEFlowBlock(mission);
		}

		public void Load(ConfigNode node)
		{
			NodePaths = new Dictionary<MENode, MENodePathInfo>();
			NodeReversePaths = new Dictionary<MENode, MENodePathInfo>();
			missionBlock = new MEFlowThenBlock(mission);
			ConfigNode node2 = new ConfigNode();
			if (node.TryGetNode("BLOCKS", ref node2))
			{
				missionBlock.Load(node2);
			}
		}

		public void Save(ConfigNode node)
		{
			ConfigNode node2 = node.AddNode("BLOCKS");
			missionBlock.Save(node2);
		}
	}
}
