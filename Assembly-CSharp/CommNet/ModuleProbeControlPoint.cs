using System.Collections.Generic;
using Expansions.Missions.Adjusters;
using KSP.Localization;

namespace CommNet
{
	public class ModuleProbeControlPoint : PartModule
	{
		[KSPField]
		public int minimumCrew = 1;

		[KSPField]
		public bool multiHop;

		private List<AdjusterProbeControlPointBase> adjusterCache = new List<AdjusterProbeControlPointBase>();

		private static string cacheAutoLOC_7000026;

		private static string cacheAutoLOC_118899;

		private static string cacheAutoLOC_6004043;

		public bool CanControl()
		{
			return !IsProbeControlPointBroken(adjusterCache);
		}

		public bool CanControlUnloaded(ProtoPartModuleSnapshot mSnap)
		{
			if (mSnap == null)
			{
				return true;
			}
			if (mSnap != null && IsProbeControlPointBroken(mSnap.GetListOfActiveAdjusters<AdjusterProbeControlPointBase>()))
			{
				return false;
			}
			return true;
		}

		protected override void OnModuleAdjusterAdded(AdjusterPartModuleBase adjuster)
		{
			if (adjuster is AdjusterProbeControlPointBase item)
			{
				adjusterCache.Add(item);
			}
			base.OnModuleAdjusterAdded(adjuster);
		}

		public override void OnModuleAdjusterRemoved(AdjusterPartModuleBase adjuster)
		{
			AdjusterProbeControlPointBase item = adjuster as AdjusterProbeControlPointBase;
			adjusterCache.Remove(item);
			base.OnModuleAdjusterRemoved(adjuster);
		}

		protected bool IsProbeControlPointBroken(List<AdjusterProbeControlPointBase> adjusterList)
		{
			int num = 0;
			while (true)
			{
				if (num < adjusterList.Count)
				{
					if (adjusterCache[num].IsProbeControlPointBroken())
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

		internal static void CacheLocalStrings()
		{
			cacheAutoLOC_7000026 = Localizer.Format("#autoLOC_7000026");
			cacheAutoLOC_118899 = Localizer.Format("#autoLOC_118899");
			cacheAutoLOC_6004043 = Localizer.Format("#autoLOC_6004043");
		}

		public override string GetInfo()
		{
			return Localizer.Format("#autoLOC_118898", minimumCrew) + (multiHop ? cacheAutoLOC_118899 : cacheAutoLOC_6004043);
		}

		public override string GetModuleDisplayName()
		{
			return cacheAutoLOC_7000026;
		}
	}
}
