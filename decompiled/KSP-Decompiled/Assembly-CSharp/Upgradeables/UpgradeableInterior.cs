using UnityEngine;

namespace Upgradeables
{
	public abstract class UpgradeableInterior : MonoBehaviour
	{
		[SerializeField]
		protected string facilityName;

		protected PSystemSetup.SpaceCenterFacility facility;

		public int FacilityLevel;

		protected void OnEnable()
		{
			facility = PSystemSetup.Instance.GetSpaceCenterFacility(facilityName);
			UpdateLevel(facility.GetFacilityLevel());
		}

		protected abstract void UpdateLevel(float normLvl);

		public abstract void SetLevel(int level);

		public abstract int GetLevelCount();
	}
}
