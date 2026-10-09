using System.Collections.Generic;
using System.ComponentModel;
using Expansions.Missions.Editor;
using KSP.Localization;
using KSP.UI;
using KSP.UI.Screens;

namespace Expansions.Missions
{
	public class VesselRestriction_Crew : VesselRestriction
	{
		public enum CrewRestrictionType
		{
			[Description("#autoLOC_8005005")]
			TotalCrew,
			[Description("#autoLOC_8005008")]
			Engineers,
			[Description("#autoLOC_8005007")]
			Scientists,
			[Description("#autoLOC_8005006")]
			Pilots
		}

		[MEGUI_Dropdown(canBePinned = false, resetValue = "TotalCrew", canBeReset = true, guiName = "#autoLOC_8100140")]
		protected CrewRestrictionType crewType;

		[MEGUI_Dropdown(canBePinned = false, resetValue = "GreaterOrEqual", canBeReset = true, guiName = "#autoLOC_8000052", Tooltip = "#autoLOC_8000053")]
		protected TestComparisonLessGreaterEqual comparisonOperator = TestComparisonLessGreaterEqual.GreaterOrEqual;

		[MEGUI_NumberRange(minValue = 0f, roundToPlaces = 0, maxValue = 100f, resetValue = "0", canBeReset = true, guiName = "#autoLOC_8100142")]
		protected int targetCrew;

		public VesselRestriction_Crew()
		{
			crewType = CrewRestrictionType.TotalCrew;
		}

		public VesselRestriction_Crew(MENode node)
			: base(node)
		{
			crewType = CrewRestrictionType.TotalCrew;
		}

		public override void SuscribeToEvents()
		{
			BaseCrewAssignmentDialog.onCrewDialogChange.Add(Callback_CrewChange);
			GameEvents.onEditorStarted.Add(UpdateAppEntry);
			GameEvents.onEditorLoad.Add(Callback_ShipLoad);
		}

		public override void ClearEvents()
		{
			BaseCrewAssignmentDialog.onCrewDialogChange.Remove(Callback_CrewChange);
			GameEvents.onEditorStarted.Remove(UpdateAppEntry);
			GameEvents.onEditorLoad.Remove(Callback_ShipLoad);
		}

		public override string GetDisplayName()
		{
			return Localizer.Format("#autoLOC_8100143");
		}

		public override void Load(ConfigNode node)
		{
			base.Load(node);
			for (int i = 0; i < node.values.Count; i++)
			{
				ConfigNode.Value value = node.values[i];
				switch (value.name)
				{
				case "crewType":
					node.TryGetEnum("crewType", ref crewType, CrewRestrictionType.TotalCrew);
					break;
				case "comparisonOperator":
					node.TryGetEnum("comparisonOperator", ref comparisonOperator, TestComparisonLessGreaterEqual.GreaterOrEqual);
					break;
				case "crewCount":
					int.TryParse(value.value, out targetCrew);
					break;
				}
			}
		}

		public override void Save(ConfigNode node)
		{
			base.Save(node);
			node.AddValue("crewType", crewType);
			node.AddValue("comparisonOperator", comparisonOperator);
			node.AddValue("crewCount", targetCrew);
		}

		public override bool SameComparator(VesselRestriction otherRestriction)
		{
			return comparisonOperator == ((VesselRestriction_Mass)otherRestriction).comparisonOperator;
		}

		public override bool IsComplete()
		{
			return VesselRestriction.TestInt(GetCrewCount(), targetCrew, comparisonOperator);
		}

		private int GetCrewCount()
		{
			int result = 0;
			switch (crewType)
			{
			case CrewRestrictionType.TotalCrew:
				if (CrewAssignmentDialog.Instance != null && CrewAssignmentDialog.Instance.CurrentManifestUnsafe != null)
				{
					result = CrewAssignmentDialog.Instance.CurrentManifestUnsafe.CrewCount;
				}
				break;
			case CrewRestrictionType.Engineers:
				result = GetCrewTraitCount(KerbalRoster.engineerTrait);
				break;
			case CrewRestrictionType.Scientists:
				result = GetCrewTraitCount(KerbalRoster.scientistTrait);
				break;
			case CrewRestrictionType.Pilots:
				result = GetCrewTraitCount(KerbalRoster.pilotTrait);
				break;
			}
			return result;
		}

		private int GetCrewTraitCount(string kerbalTrait)
		{
			int num = 0;
			if (CrewAssignmentDialog.Instance != null && CrewAssignmentDialog.Instance.CurrentManifestUnsafe != null)
			{
				List<ProtoCrewMember> allCrew = CrewAssignmentDialog.Instance.CurrentManifestUnsafe.GetAllCrew(includeNulls: false);
				for (int i = 0; i < allCrew.Count; i++)
				{
					if (allCrew[i] != null && allCrew[i].trait == kerbalTrait)
					{
						num++;
					}
				}
			}
			return num;
		}

		public override string GetStateMessage()
		{
			switch (comparisonOperator)
			{
			default:
				return string.Empty;
			case TestComparisonLessGreaterEqual.LessOrEqual:
				return Localizer.Format("#autoLOC_8100145", targetCrew, crewType.Description(), GetCrewCount());
			case TestComparisonLessGreaterEqual.Equal:
				return Localizer.Format("#autoLOC_8100144", targetCrew, crewType.Description(), GetCrewCount());
			case TestComparisonLessGreaterEqual.GreaterOrEqual:
				return Localizer.Format("#autoLOC_8100146", targetCrew, crewType.Description(), GetCrewCount());
			}
		}

		private void Callback_CrewChange(VesselCrewManifest newCrew)
		{
			UpdateAppEntry();
		}

		private void Callback_ShipLoad(ShipConstruct ct, CraftBrowserDialog.LoadType loadType)
		{
			UpdateAppEntry();
		}
	}
}
