using System;
using System.Collections.Generic;
using UnityEngine;

namespace Contracts.Agents
{
	public class AgentMentality
	{
		private static List<Type> mentalityTypes;

		[SerializeField]
		private Agent agent;

		[SerializeField]
		private float factor;

		public Agent Agent => agent;

		public float Factor => factor;

		public string Description => GetDescription();

		public string DisplayName => GetDisplayName();

		private static void GenerateMentalityTypes()
		{
			if (mentalityTypes != null)
			{
				return;
			}
			mentalityTypes = new List<Type>();
			AssemblyLoader.loadedAssemblies.TypeOperation(delegate(Type t)
			{
				if (t.IsSubclassOf(typeof(AgentMentality)) && !(t == typeof(AgentMentality)))
				{
					mentalityTypes.Add(t);
				}
			});
			Debug.Log("[Agent]: Found " + mentalityTypes.Count + " agent mentality types");
		}

		public static Type GetMentalityType(string typeName)
		{
			int num = 0;
			int count = mentalityTypes.Count;
			while (true)
			{
				if (num < count)
				{
					if (mentalityTypes[num].Name == typeName)
					{
						break;
					}
					num++;
					continue;
				}
				return null;
			}
			return mentalityTypes[num];
		}

		protected virtual string GetDescription()
		{
			return null;
		}

		protected virtual string GetDisplayName()
		{
			return GetType().Name;
		}

		public virtual KeywordScore ScoreKeyword(string keyword)
		{
			return KeywordScore.None;
		}

		public virtual bool CanProcessContract(Contract contract)
		{
			return true;
		}

		public virtual void ProcessContract(Contract contract)
		{
		}

		public virtual void OnAdded()
		{
		}

		public static AgentMentality AddMentality(Agent agent, string mentalityName, float factor)
		{
			GenerateMentalityTypes();
			Type mentalityType = GetMentalityType(mentalityName);
			if (mentalityType == null)
			{
				Debug.LogError("[Agent]: Cannot load mentality, no mentality with name '" + mentalityName + "'.");
				return null;
			}
			int num = 0;
			while (true)
			{
				if (num < agent.Mentality.Count)
				{
					if (agent.Mentality[num].GetType() == mentalityType)
					{
						break;
					}
					num++;
					continue;
				}
				AgentMentality agentMentality = (AgentMentality)Activator.CreateInstance(mentalityType);
				agentMentality.agent = agent;
				agentMentality.factor = Mathf.Clamp(factor, 0f, 1f);
				agent.Mentality.Add(agentMentality);
				agentMentality.OnAdded();
				return agentMentality;
			}
			Debug.LogError("[Agent]: Cannot load mentality of name '" + mentalityName + "'. Agent already has a mentality of that type.");
			return null;
		}

		protected void FactorFundsAdvance(Contract contract, bool positive)
		{
			float num = GameVariables.Instance.GetMentalityFundsFactor(Factor, contract.Prestige);
			if (!positive)
			{
				num = 1f / num;
			}
			contract.FundsAdvance *= num;
		}

		protected void FactorFundsCompletion(Contract contract, bool positive)
		{
			float num = GameVariables.Instance.GetMentalityFundsFactor(Factor, contract.Prestige);
			if (!positive)
			{
				num = 1f / num;
			}
			contract.FundsCompletion *= num;
		}

		protected void FactorFundsFailure(Contract contract, bool positive)
		{
			float num = GameVariables.Instance.GetMentalityFundsFactor(Factor, contract.Prestige);
			if (!positive)
			{
				num = 1f / num;
			}
			contract.FundsFailure *= num;
		}

		protected void FactorScienceCompletion(Contract contract, bool positive)
		{
			float num = GameVariables.Instance.GetMentalityScienceFactor(Factor, contract.Prestige);
			if (!positive)
			{
				num = 1f / num;
			}
			contract.ScienceCompletion *= num;
		}

		protected void FactorReputationCompletion(Contract contract, bool positive)
		{
			float num = GameVariables.Instance.GetMentalityReputationFactor(Factor, contract.Prestige);
			if (!positive)
			{
				num = 1f / num;
			}
			contract.ReputationCompletion *= num;
		}

		protected void FactorReputationFailure(Contract contract, bool positive)
		{
			float num = GameVariables.Instance.GetMentalityReputationFactor(Factor, contract.Prestige);
			if (!positive)
			{
				num = 1f / num;
			}
			contract.ReputationFailure *= num;
		}

		protected void FactorDeadline(Contract contract, bool positive)
		{
			float num = GameVariables.Instance.GetMentalityDeadlineFactor(Factor, contract.Prestige);
			if (!positive)
			{
				num = 1f / num;
			}
			contract.TimeDeadline *= num;
		}

		protected void FactorExpiry(Contract contract, bool positive)
		{
			float num = GameVariables.Instance.GetMentalityExpiryFactor(Factor, contract.Prestige);
			if (!positive)
			{
				num = 1f / num;
			}
			contract.TimeExpiry *= num;
		}
	}
}
