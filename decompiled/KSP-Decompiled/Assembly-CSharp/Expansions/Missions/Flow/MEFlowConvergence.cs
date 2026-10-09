namespace Expansions.Missions.Flow
{
	public class MEFlowConvergence
	{
		public int Hops { get; private set; }

		public MENode Node { get; private set; }

		public MEFlowConvergence(MENode node, int hops)
		{
			Node = node;
			Hops = hops;
		}
	}
}
