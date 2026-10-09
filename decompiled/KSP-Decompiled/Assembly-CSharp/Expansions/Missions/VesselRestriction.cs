using Expansions.Missions.Runtime;
using KSP.Localization;
using KSP.UI;
using PreFlightTests;
using TMPro;
using UnityEngine;

namespace Expansions.Missions
{
	public class VesselRestriction : DynamicModule
	{
		protected UIListItem UIEntry;

		private UIStateImage UIEntryState;

		private static double epsilon = 1E-15;

		public VesselRestriction()
		{
		}

		public VesselRestriction(MENode node)
			: base(node)
		{
		}

		public virtual void SuscribeToEvents()
		{
		}

		public virtual void ClearEvents()
		{
		}

		public override string GetDisplayName()
		{
			return Localizer.Format("#autoLOC_8100211");
		}

		public void AddAppUIReference(ref UIListItem listItem, ref UIStateImage listItemState)
		{
			UIEntry = listItem;
			UIEntryState = listItemState;
			UpdateAppEntry();
		}

		public void ClearAppUIREference()
		{
			UIEntry = null;
			UIEntryState = null;
		}

		public virtual void UpdateAppEntry()
		{
			if (UIEntry != null && UIEntryState != null)
			{
				UIEntry.GetComponentInChildren<TextMeshProUGUI>().text = GetStateMessage();
				int state = ((!IsComplete()) ? 1 : 0);
				UIEntryState.SetState(state);
				if (!IsComplete() && UIEntry.Data is MissionsAppVesselInfo missionsAppVesselInfo)
				{
					missionsAppVesselInfo.vesselSituation.readyToLaunch = false;
					MissionsApp.Instance.SetVesselItemReadyIcon(missionsAppVesselInfo.vesselSituation, missionsAppVesselInfo);
				}
			}
		}

		public virtual string GetStateMessage()
		{
			return Localizer.Format("#autoLOC_8100212", Time.time);
		}

		public virtual bool IsComplete()
		{
			return true;
		}

		internal bool SameComparatorWrapper(VesselRestriction otherRestriction)
		{
			if (GetType() == otherRestriction.GetType())
			{
				return SameComparator(otherRestriction);
			}
			return false;
		}

		public virtual bool SameComparator(VesselRestriction otherRestriction)
		{
			return true;
		}

		public virtual IPreFlightTest GetPreflightCheck()
		{
			return new MissionPreflightCheck(this);
		}

		protected static bool TestFloat(float currentValue, float targetValue, TestComparisonLessGreaterEqual comparisonOperator)
		{
			switch (comparisonOperator)
			{
			default:
				return true;
			case TestComparisonLessGreaterEqual.LessOrEqual:
				return currentValue <= targetValue;
			case TestComparisonLessGreaterEqual.Equal:
				return (double)Mathf.Abs(targetValue - currentValue) < epsilon;
			case TestComparisonLessGreaterEqual.GreaterOrEqual:
				return currentValue >= targetValue;
			}
		}

		public static bool TestInt(int currentValue, int targetValue, TestComparisonLessGreaterEqual comparisonOperator)
		{
			switch (comparisonOperator)
			{
			default:
				return true;
			case TestComparisonLessGreaterEqual.LessOrEqual:
				return currentValue <= targetValue;
			case TestComparisonLessGreaterEqual.Equal:
				return targetValue - currentValue == 0;
			case TestComparisonLessGreaterEqual.GreaterOrEqual:
				return currentValue >= targetValue;
			}
		}
	}
}
