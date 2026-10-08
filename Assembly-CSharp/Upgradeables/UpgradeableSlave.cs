using UnityEngine;

namespace Upgradeables
{
	public class UpgradeableSlave : UpgradeableObject
	{
		[SerializeField]
		private UpgradeableFacility[] neighbours;

		[SerializeField]
		private string[] neighbourIDs;

		private float neighbourAvgNorm;

		public UpgradeableFacility[] Neighbours => neighbours;

		public string[] NeighbourIDs => neighbourIDs;

		[ContextMenu("Compile Neighbour IDs")]
		public void CompileIDs()
		{
			neighbourIDs = new string[neighbours.Length];
			int num = neighbours.Length;
			while (num-- > 0)
			{
				if (neighbours[num] != null)
				{
					neighbourIDs[num] = neighbours[num].id;
				}
			}
		}

		protected override void OnAwake()
		{
			GameEvents.OnUpgradeableObjLevelChange.Add(OnObjectLevelChange);
		}

		protected override void OnStart()
		{
			StartCoroutine(CallbackUtil.DelayedCallback(3, delegate
			{
				UpdateLevel();
			}));
		}

		protected override void OnOnDestroy()
		{
			GameEvents.OnUpgradeableObjLevelChange.Remove(OnObjectLevelChange);
		}

		private void OnObjectLevelChange(UpgradeableObject upObj, int lvl)
		{
			UpgradeableFacility upgradeableFacility = upObj as UpgradeableFacility;
			if (!(upgradeableFacility != null))
			{
				return;
			}
			int num = neighbourIDs.Length;
			do
			{
				if (num-- <= 0)
				{
					return;
				}
			}
			while (!(neighbourIDs[num] == upgradeableFacility.id));
			UpdateLevel();
		}

		private void UpdateLevel()
		{
			neighbourAvgNorm = 0f;
			int num = neighbourIDs.Length;
			while (num-- > 0)
			{
				string facilityId = neighbourIDs[num];
				neighbourAvgNorm += ScenarioUpgradeableFacilities.GetFacilityLevel(facilityId);
			}
			if (neighbourIDs.Length > 1)
			{
				neighbourAvgNorm /= neighbourIDs.Length;
			}
			facilityLevel = Mathf.FloorToInt(neighbourAvgNorm * (float)upgradeLevels.Length);
			facilityLevel = Mathf.Clamp(facilityLevel, 0, upgradeLevels.Length - 1);
			setLevel(facilityLevel);
		}
	}
}
