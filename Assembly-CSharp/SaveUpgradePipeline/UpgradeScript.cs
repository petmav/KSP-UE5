using System;
using UnityEngine;

namespace SaveUpgradePipeline
{
	public abstract class UpgradeScript
	{
		public LoadContext ContextMask;

		protected string nodeUrlCraft;

		protected string nodeUrlSFS;

		public abstract string Name { get; }

		public abstract string Description { get; }

		public abstract Version EarliestCompatibleVersion { get; }

		public abstract Version TargetVersion { get; }

		public void Init(LoadContext contextMask, string nodeUrlCraft, string nodeUrlSFS)
		{
			this.nodeUrlCraft = nodeUrlCraft;
			this.nodeUrlSFS = nodeUrlSFS;
			ContextMask = contextMask;
			OnInit();
		}

		public bool AppliesInContext(LoadContext ctx)
		{
			return (ctx & ContextMask) != 0;
		}

		public abstract TestResult OnTest(ConfigNode node, LoadContext loadContext, ref string nodeName);

		public abstract void OnUpgrade(ConfigNode node, LoadContext loadContext, ConfigNode parentNode);

		protected virtual void OnInit()
		{
		}

		public virtual TestResult Test(ConfigNode n, LoadContext loadContext)
		{
			TestResult tRst = VersionTest(GetCfgNodeVersion(n, loadContext));
			if (tRst != TestResult.Upgradeable)
			{
				return tRst;
			}
			string nodeName = string.Empty;
			string nodeURL = GetNodeURL(loadContext);
			if (string.IsNullOrEmpty(nodeURL))
			{
				tRst = OnTest(n, loadContext, ref nodeName);
				LogTestResults(nodeName, tRst);
				return tRst;
			}
			tRst = TestResult.Pass;
			RecurseNodes(n, nodeURL.Split('/'), 0, delegate(ConfigNode node, ConfigNode parent)
			{
				nodeName = string.Empty;
				TestResult testResult = OnTest(node, loadContext, ref nodeName);
				LogTestResults(nodeName, testResult);
				switch (testResult)
				{
				case TestResult.TooEarly:
					throw new InvalidOperationException("Script-Level testing shouldn't return TooEarly. This value is only meaningful for Version testing. Override VersionTest if necessary.");
				case TestResult.Upgradeable:
					if (tRst != TestResult.Pass)
					{
						break;
					}
					goto case TestResult.Failed;
				case TestResult.Failed:
					tRst = testResult;
					break;
				}
			});
			return tRst;
		}

		public virtual void Upgrade(ConfigNode n, LoadContext loadContext)
		{
			string nodeName = string.Empty;
			string nodeURL = GetNodeURL(loadContext);
			if (string.IsNullOrEmpty(nodeURL))
			{
				if (OnTest(n, loadContext, ref nodeName) == TestResult.Upgradeable)
				{
					OnUpgrade(n, loadContext, n);
				}
			}
			else
			{
				RecurseNodes(n, nodeURL.Split('/'), 0, delegate(ConfigNode node, ConfigNode parent)
				{
					nodeName = string.Empty;
					if (OnTest(node, loadContext, ref nodeName) == TestResult.Upgradeable)
					{
						OnUpgrade(node, loadContext, parent);
					}
				});
			}
			n.AddValue("upgraded", Name, Description);
		}

		protected virtual bool CheckMinVersion(Version v)
		{
			return v >= EarliestCompatibleVersion;
		}

		protected virtual bool CheckMaxVersion(Version v)
		{
			return v < TargetVersion;
		}

		protected virtual Version GetCfgNodeVersion(ConfigNode n, LoadContext loadContext)
		{
			return NodeUtil.GetCfgVersion(n, loadContext);
		}

		protected virtual TestResult VersionTest(Version v)
		{
			if (!CheckMinVersion(v))
			{
				Debug.LogWarning("[UpgradeScript]: " + Name + " supports files from " + EarliestCompatibleVersion.ToString() + " and higher only!");
				return TestResult.TooEarly;
			}
			if (!CheckMaxVersion(v))
			{
				return TestResult.Pass;
			}
			return TestResult.Upgradeable;
		}

		protected string GetNodeURL(LoadContext loadContext)
		{
			switch (loadContext)
			{
			default:
				return null;
			case LoadContext.Craft:
				return nodeUrlCraft;
			case LoadContext.SFS:
				return nodeUrlSFS;
			}
		}

		protected void RecurseNodes(ConfigNode node, string[] urlNodes, int level, Callback<ConfigNode, ConfigNode> cb, ConfigNode parent = null)
		{
			if (level < urlNodes.Length)
			{
				int num = node.GetNodes(urlNodes[level]).Length;
				while (num-- > 0)
				{
					RecurseNodes(node.GetNodes(urlNodes[level])[num], urlNodes, level + 1, cb, node);
				}
			}
			else
			{
				cb(node, parent);
			}
		}

		protected void LogTestResults(string nodeName, TestResult test)
		{
			switch (test)
			{
			case TestResult.Failed:
				Debug.LogWarning("[UpgradeScript]: Module " + Name + " Failed for " + nodeName + "!");
				break;
			default:
				Debug.LogError("[UpgradeScript]: Module " + Name + " test was undefined  for " + nodeName + "!");
				break;
			case TestResult.Upgradeable:
			case TestResult.Pass:
				break;
			}
		}

		public override string ToString()
		{
			return string.Concat(Name, " [", EarliestCompatibleVersion, " > ", TargetVersion, "]");
		}
	}
}
