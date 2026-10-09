using UnityEngine.EventSystems;

namespace KSP.UI.Screens
{
	public abstract class KbAppVessel : KbApp
	{
		protected Vessel vessel;

		public override void Awake()
		{
			base.Awake();
			GameEvents.onVesselRename.Add(OnVesselRename);
			GameEvents.onInputLocksModified.Add(OnInputLocksChange);
		}

		public override void OnDestroy()
		{
			base.OnDestroy();
			GameEvents.onVesselRename.Remove(OnVesselRename);
			GameEvents.onInputLocksModified.Remove(OnInputLocksChange);
		}

		public override void Setup()
		{
			base.Setup();
			appFrame.headerClickHandler.onPointerClick.AddListener(OnPanelHeaderTap);
		}

		protected override void DisplayApp()
		{
		}

		protected override void HideApp()
		{
		}

		private void OnVesselRename(GameEvents.HostedFromToAction<Vessel, string> nChg)
		{
			if (vessel == nChg.host)
			{
				appFrame.appName.text = vessel.DiscoveryInfo.displayName.Value;
			}
		}

		public void OnPanelHeaderTap(PointerEventData eventData)
		{
			if (InputLockManager.IsUnlocked(ControlTypes.EDITOR_EDIT_NAME_FIELDS) && vessel != null && eventData.clickCount == 2 && vessel.vesselType != VesselType.EVA && vessel.vesselType != VesselType.Flag)
			{
				appFrame.appName.text = "|";
				vessel.RenameVessel();
			}
		}

		private void OnInputLocksChange(GameEvents.FromToAction<ControlTypes, ControlTypes> iChg)
		{
			if (InputLockManager.IsUnlocked(ControlTypes.EDITOR_EDIT_NAME_FIELDS) && vessel != null)
			{
				appFrame.appName.text = vessel.DiscoveryInfo.displayName.Value;
			}
		}
	}
}
