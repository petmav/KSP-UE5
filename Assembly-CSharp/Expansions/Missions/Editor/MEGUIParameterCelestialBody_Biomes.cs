using System;

namespace Expansions.Missions.Editor
{
	[MEGUI_CelestialBody_Biomes]
	public class MEGUIParameterCelestialBody_Biomes : MEGUICompoundParameter
	{
		private GAPCelestialBody gapCB;

		private MEGUIParameterDropdownList dropDownBodies;

		private MEGUIParameterDropdownList dropDownBiomes;

		public MissionBiome FieldValue
		{
			get
			{
				return (MissionBiome)field.GetValue();
			}
			set
			{
				field.SetValue(value);
			}
		}

		protected override void Setup(string name)
		{
			base.Setup(name);
			title.text = name;
			dropDownBodies = subParameters["body"] as MEGUIParameterDropdownList;
			dropDownBiomes = subParameters["biomeName"] as MEGUIParameterDropdownList;
			dropDownBodies.dropdownList.onValueChanged.AddListener(OnDropDownBody);
			dropDownBiomes.dropdownList.onValueChanged.AddListener(OnDropDownBiome);
		}

		private void UpdateValue(int bodyIndex, int biomeIndex)
		{
			CelestialBody celestialBody = FlightGlobals.Bodies[bodyIndex];
			string empty = string.Empty;
			if (celestialBody.BiomeMap != null && biomeIndex > 0)
			{
				empty = celestialBody.BiomeMap.Attributes[biomeIndex - 1].name;
			}
			FieldValue = new MissionBiome(celestialBody, empty);
		}

		private void OnDropDownBody(int value)
		{
			dropDownBiomes.RebuildDropDown();
			dropDownBiomes.dropdownList.value = 0;
			gapCB.Load(FieldValue.body);
		}

		private void OnDropDownBiome(int value)
		{
			gapCB.Biomes.SelectBiomeByName(FieldValue.biomeName);
		}

		private void OnGAPBiomeSelected(CBAttributeMapSO.MapAttribute biome)
		{
			if (biome != null)
			{
				int itemIndex = dropDownBiomes.GetItemIndex(biome.name);
				dropDownBiomes.dropdownList.value = itemIndex;
			}
			else
			{
				dropDownBiomes.dropdownList.value = 0;
			}
		}

		private void OnGapLeftArrow()
		{
			int itemIndex = dropDownBodies.GetItemIndex(FieldValue.body);
			itemIndex = (itemIndex + dropDownBodies.dropdownList.options.Count - 1) % dropDownBodies.dropdownList.options.Count;
			dropDownBodies.dropdownList.value = itemIndex;
		}

		private void OnGapRightArrow()
		{
			int itemIndex = dropDownBodies.GetItemIndex(FieldValue.body);
			itemIndex = (itemIndex + 1) % dropDownBodies.dropdownList.options.Count;
			dropDownBodies.dropdownList.value = itemIndex;
		}

		public override void DisplayGAP()
		{
			base.DisplayGAP();
			gapCB = MissionEditorLogic.Instance.actionPane.GAPInitialize<GAPCelestialBody>();
			gapCB.SetState(GAPCelestialBodyState.BIOMES);
			gapCB.SuscribeToLeftButton(OnGapLeftArrow);
			gapCB.SuscribeToRightButton(OnGapRightArrow);
			GAPCelestialBodyState_Biomes biomes = gapCB.Biomes;
			biomes.OnGAPBiomeSelected = (GAPCelestialBodyState_Biomes.OnBiomeSelected)Delegate.Combine(biomes.OnGAPBiomeSelected, new GAPCelestialBodyState_Biomes.OnBiomeSelected(OnGAPBiomeSelected));
			gapCB.Load(FieldValue.body);
			gapCB.Biomes.SelectBiomeByName(FieldValue.biomeName);
		}
	}
}
