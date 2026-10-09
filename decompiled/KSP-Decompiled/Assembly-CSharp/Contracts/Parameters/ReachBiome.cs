using System;

namespace Contracts.Parameters
{
	[Serializable]
	public class ReachBiome : ContractParameter
	{
		public string BiomeName;

		protected string title = "";

		private bool trackerActive;

		private bool IsWithinBiome;

		public ReachBiome()
		{
		}

		public ReachBiome(string biomeName, string title)
		{
			BiomeName = biomeName ?? "";
			this.title = title;
		}

		protected override string GetTitle()
		{
			return title + BiomeName;
		}

		public static string GetTitleStringShort(string biomeName)
		{
			return biomeName;
		}

		protected override void OnReset()
		{
			SetIncomplete();
		}

		protected override void OnLoad(ConfigNode node)
		{
			if (node.HasValue("biome"))
			{
				BiomeName = node.GetValue("biome");
			}
			if (node.HasValue("title"))
			{
				title = node.GetValue("title");
			}
		}

		protected override void OnSave(ConfigNode node)
		{
			node.AddValue("biome", BiomeName);
			node.AddValue("title", title);
		}

		protected override void OnUpdate()
		{
			if (base.Root.ContractState == Contract.State.Active && HighLogic.LoadedSceneIsFlight && FlightGlobals.ready)
			{
				IsWithinBiome = false;
				if (bodyHasBiome(FlightGlobals.ActiveVessel.mainBody))
				{
					IsWithinBiome = checkVesselWithinBiome(FlightGlobals.ActiveVessel);
				}
				if (IsWithinBiome && base.State == ParameterState.Incomplete)
				{
					SetComplete();
				}
				if (!IsWithinBiome && base.State == ParameterState.Complete)
				{
					SetIncomplete();
				}
			}
		}

		private bool checkVesselWithinBiome(Vessel v)
		{
			if (!string.IsNullOrEmpty(BiomeName))
			{
				if (v.mainBody.BiomeMap != null)
				{
					return v.mainBody.BiomeMap.GetAtt(v.latitude, v.longitude).name == BiomeName;
				}
				return false;
			}
			return true;
		}

		private bool bodyHasBiome(CelestialBody body)
		{
			if (!string.IsNullOrEmpty(BiomeName) && !(body.BiomeMap == null))
			{
				int num = body.BiomeMap.Attributes.Length;
				do
				{
					if (num-- <= 0)
					{
						return false;
					}
				}
				while (!(body.BiomeMap.Attributes[num].name == BiomeName));
				return true;
			}
			return false;
		}
	}
}
