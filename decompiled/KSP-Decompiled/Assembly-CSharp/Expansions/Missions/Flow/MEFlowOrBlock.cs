namespace Expansions.Missions.Flow
{
	public class MEFlowOrBlock : MEFlowBlock
	{
		internal Callback<MEFlowParser> OnUpdateFlowUI;

		public MEFlowOrBlock(Mission mission)
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
