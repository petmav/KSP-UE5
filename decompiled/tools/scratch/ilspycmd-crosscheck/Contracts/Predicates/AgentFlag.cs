using System;
using Contracts.Agents;
using KSP.Localization;
using UnityEngine;

namespace Contracts.Predicates
{
	[Serializable]
	public class AgentFlag : ContractPredicate
	{
		private Agent agent;

		public Agent Agent => agent;

		public AgentFlag(IContractParameterHost parent)
			: base(parent)
		{
		}

		public AgentFlag(IContractParameterHost contract, Agent agent)
			: base(contract)
		{
			this.agent = agent;
		}

		public override bool Test(Vessel vessel)
		{
			int num = 0;
			while (true)
			{
				if (num < vessel.parts.Count)
				{
					if (vessel.parts[num].flagURL == agent.LogoURL)
					{
						break;
					}
					num++;
					continue;
				}
				return false;
			}
			return true;
		}

		public override bool Test(ProtoVessel vessel)
		{
			int num = 0;
			while (true)
			{
				if (num < vessel.protoPartSnapshots.Count)
				{
					if (vessel.protoPartSnapshots[num].flagURL == agent.LogoURL)
					{
						break;
					}
					num++;
					continue;
				}
				return false;
			}
			return true;
		}

		protected override string GetDescription()
		{
			return Localizer.Format("#autoLOC_270961", agent.Title);
		}

		protected override void OnLoad(ConfigNode node)
		{
			if (!node.HasValue("agent"))
			{
				Debug.LogError("Contract Predicate " + GetType().Name + " has invalid config.");
				return;
			}
			agent = AgentList.Instance.GetAgent(node.GetValue("agent"));
			if (agent == null)
			{
				Debug.LogError("Contract Predicate " + GetType().Name + " has an invalid agent name.");
			}
		}

		protected override void OnSave(ConfigNode node)
		{
			node.AddValue("agent", agent.Name);
		}
	}
}
