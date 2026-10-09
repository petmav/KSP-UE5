namespace Expansions.Missions
{
	public class NodeLabel : BaseLabel
	{
		public MENode node { get; private set; }

		public ITestNodeLabel testModule { get; private set; }

		public void Setup(VesselLabels labels, ITestNodeLabel newTestModule)
		{
			Setup(labels, labels.GetLabelType(newTestModule));
			node = newTestModule.node;
			testModule = newTestModule;
		}
	}
}
