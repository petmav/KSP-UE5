namespace SaveUpgradePipeline
{
	public abstract class ModuleReplace : UpgradeScript
	{
		protected string moduleName;

		protected string newModuleName;

		private ConfigNode[] moduleNodes;

		protected override void OnInit()
		{
			moduleNodes = null;
			Setup(out moduleName, out newModuleName);
		}

		protected abstract void Setup(out string moduleName, out string newModuleName);

		public override TestResult OnTest(ConfigNode node, LoadContext loadContext, ref string nodeName)
		{
			nodeName = moduleName;
			moduleNodes = NodeUtil.GetModuleNodesNamed(node, moduleName);
			if (moduleNodes != null && moduleNodes.Length != 0)
			{
				return TestResult.Upgradeable;
			}
			return TestResult.Pass;
		}

		public override void OnUpgrade(ConfigNode node, LoadContext loadContext, ConfigNode parentNode)
		{
			for (int i = 0; i < moduleNodes.Length; i++)
			{
				if (moduleNodes[i].HasValue("name"))
				{
					moduleNodes[i].SetValue("name", newModuleName);
				}
			}
		}
	}
}
