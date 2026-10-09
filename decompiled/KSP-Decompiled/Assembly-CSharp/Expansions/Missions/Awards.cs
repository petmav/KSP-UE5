using System;
using System.Collections.Generic;
using UnityEngine;

namespace Expansions.Missions
{
	[Serializable]
	public class Awards : MonoBehaviour
	{
		[SerializeField]
		private List<AwardDefinition> internalAwards = new List<AwardDefinition>();

		private Dictionary<string, AwardDefinition> awardsDictionary;

		public List<AwardDefinition> AwardDefinitions { get; protected set; }

		private void Awake()
		{
			awardsDictionary = new Dictionary<string, AwardDefinition>();
			AwardDefinitions = new List<AwardDefinition>();
			SetupAwards();
		}

		private void SetupAwards()
		{
			AwardDefinitions.AddRange(internalAwards);
			int i = 0;
			for (int count = internalAwards.Count; i < count; i++)
			{
				awardsDictionary.Add(internalAwards[i].id, internalAwards[i]);
			}
		}

		public AwardDefinition GetAwardDefinition(string id)
		{
			if (awardsDictionary.ContainsKey(id))
			{
				return awardsDictionary[id];
			}
			return null;
		}
	}
}
