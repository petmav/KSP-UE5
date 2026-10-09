using FinePrint;
using FinePrint.Utilities;
using KSP.Localization;

namespace KSPAchievements
{
	public class RecordsSpeed : ProgressNode
	{
		public double record;

		private Vessel recordHolder;

		private double rewardThreshold;

		private int rewardInterval = 1;

		private double speedCeiling = -1.0;

		private double maxSpeed
		{
			get
			{
				if (speedCeiling >= 0.0)
				{
					return speedCeiling;
				}
				if (ContractDefs.Instance == null)
				{
					return 2500.0;
				}
				speedCeiling = ContractDefs.Progression.MaxSpeedRecord;
				return speedCeiling;
			}
		}

		public RecordsSpeed()
			: base("RecordsSpeed", startReached: false)
		{
			OnIterateVessels = iterateVessels;
		}

		private void iterateVessels(Vessel v)
		{
			if (v == null || v != FlightGlobals.ActiveVessel || !v.isCommandable || v.DiscoveryInfo.Level != DiscoveryLevels.Owned || !(v.mainBody.name == Planetarium.fetch.Home.name))
			{
				return;
			}
			double num = 0.0;
			switch (v.situation)
			{
			default:
				num = v.obt_speed;
				break;
			case Vessel.Situations.PRELAUNCH:
				return;
			case Vessel.Situations.LANDED:
			case Vessel.Situations.SPLASHED:
			case Vessel.Situations.FLYING:
				num = v.srfSpeed;
				break;
			}
			if (num > record)
			{
				if (num > maxSpeed)
				{
					record = maxSpeed;
				}
				else
				{
					record = num;
				}
				if (recordHolder != v)
				{
					Reach();
					recordHolder = v;
				}
				else
				{
					AchieveDate = Planetarium.GetUniversalTime();
					Achieve();
				}
			}
			if (!base.IsComplete)
			{
				if (rewardThreshold <= 0.0)
				{
					rewardThreshold = ProgressUtilities.FindNextRecord(0.0, maxSpeed, 5.0, ref rewardInterval);
				}
				while (record >= rewardThreshold)
				{
					AwardProgressInterval(Localizer.Format("#autoLOC_298174", StringUtilities.ValueAndUnits(RecordTrackType.SPEED, rewardThreshold)), rewardInterval, ContractDefs.Progression.RecordSplit, ProgressType.SPEEDRECORD);
					if (rewardInterval >= ContractDefs.Progression.RecordSplit || rewardThreshold >= maxSpeed)
					{
						break;
					}
					rewardThreshold = ProgressUtilities.FindNextRecord(rewardThreshold, maxSpeed, 5.0, ref rewardInterval);
				}
			}
			if (num >= maxSpeed)
			{
				Complete();
				OnIterateVessels = null;
			}
		}

		protected override void OnLoad(ConfigNode node)
		{
			if (node.HasValue("record"))
			{
				record = double.Parse(node.GetValue("record"));
			}
			if (base.IsComplete)
			{
				OnIterateVessels = null;
			}
			else
			{
				rewardThreshold = ProgressUtilities.FindNextRecord(record, maxSpeed, 5.0, ref rewardInterval);
			}
		}

		protected override void OnSave(ConfigNode node)
		{
			node.AddValue("record", record);
		}
	}
}
