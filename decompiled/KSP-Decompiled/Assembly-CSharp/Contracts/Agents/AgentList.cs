using System.Collections.Generic;
using KSP.Localization;
using UnityEngine;

namespace Contracts.Agents
{
	public class AgentList : MonoBehaviour
	{
		[SerializeField]
		private int logoWidth = 64;

		[SerializeField]
		private int logoHeight = 40;

		[SerializeField]
		private List<Agent> agencies = new List<Agent>();

		private static int suitabilityFactor = 2;

		public int LogoWidth => logoWidth;

		public int LogoHeight => logoHeight;

		public static AgentList Instance { get; private set; }

		public List<Agent> Agencies => agencies;

		private void Awake()
		{
			Instance = this;
		}

		private void Start()
		{
			ConfigNode[] configNodes = GameDatabase.Instance.GetConfigNodes("AGENT");
			LoadAgents(configNodes);
		}

		private void OnDestroy()
		{
			if (Instance != null && Instance == this)
			{
				Instance = null;
			}
		}

		public void LoadAgents(ConfigNode[] nodes)
		{
			int i = 0;
			for (int num = nodes.Length; i < num; i++)
			{
				Agent agent = Agent.LoadAgent(nodes[i]);
				if (agent != null)
				{
					agencies.Add(agent);
				}
			}
			int j = 0;
			for (int count = agencies.Count; j < count; j++)
			{
				int count2 = agencies[j].Standings.Count;
				while (count2-- > 0)
				{
					if (!agencies[j].Standings[count2].Link())
					{
						agencies[j].Standings.RemoveAt(count2);
					}
				}
			}
			if (agencies.Count == 0)
			{
				agencies.Add(new Agent("Kerbal Space Program", "Kerbal Space Program", "", ""));
			}
			Debug.Log("[AgentList]: " + agencies.Count + " agents parsed and loaded.");
		}

		public Agent GetAgent(string name)
		{
			int num = 0;
			int count = agencies.Count;
			while (true)
			{
				if (num < count)
				{
					if (agencies[num].Name == name)
					{
						break;
					}
					num++;
					continue;
				}
				return null;
			}
			return agencies[num];
		}

		public Agent GetAgentbyTitle(string title)
		{
			int num = 0;
			int count = agencies.Count;
			while (true)
			{
				if (num < count)
				{
					if (Localizer.Format(agencies[num].Title) == title)
					{
						break;
					}
					num++;
					continue;
				}
				return null;
			}
			return agencies[num];
		}

		public Agent GetAgentRandom()
		{
			return agencies[Random.Range(0, agencies.Count)];
		}

		public Agent GetSuitableAgentForContract(Contract contract)
		{
			List<Agent> list = new List<Agent>();
			int i = 0;
			for (int count = agencies.Count; i < count; i++)
			{
				if (!agencies[i].CanProcessContract(contract))
				{
					continue;
				}
				int num = agencies[i].ScoreAgentSuitability(contract) * suitabilityFactor;
				if (num > 0)
				{
					for (int j = 0; j < num; j++)
					{
						list.Add(agencies[i]);
					}
				}
			}
			if (list.Count == 0)
			{
				return null;
			}
			return list[Random.Range(0, list.Count)];
		}

		public Agent GetPartManufacturer(AvailablePart ap)
		{
			if (!string.IsNullOrEmpty(ap.manufacturer))
			{
				int i = 0;
				for (int count = agencies.Count; i < count; i++)
				{
					if (agencies[i].Name == ap.manufacturer || agencies[i].Name.Contains(ap.manufacturer) || ap.manufacturer.Contains(agencies[i].Name))
					{
						return agencies[i];
					}
				}
			}
			return GetAgent(Localizer.Format("#autoLOC_6002413"));
		}
	}
}
