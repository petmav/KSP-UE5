namespace Expansions.Missions.Flow
{
	public class MEFlowThenBlock : MEFlowBlock
	{
		internal Callback<MEFlowParser> OnUpdateFlowUI;

		public MEFlowThenBlock(Mission mission)
			: base(mission)
		{
		}

		internal new void UpdateMissionFlowUI(MEFlowParser parser)
		{
			if (OnUpdateFlowUI != null)
			{
				OnUpdateFlowUI(parser);
			}
		}
	}
}
