using UnityEngine.EventSystems;

namespace KSP.UI.Screens
{
	public abstract class KbAppUnowned : KbApp
	{
		protected IDiscoverable currentUnowned;

		public override void Awake()
		{
			base.Awake();
			GameEvents.onVesselRename.Add(onVesselRename);
			GameEvents.onInputLocksModified.Add(OnInputLocksChange);
			GameEvents.onKnowledgeChanged.Add(OnKnowledgeChange);
		}

		public override void OnDestroy()
		{
			base.OnDestroy();
			GameEvents.onVesselRename.Remove(onVesselRename);
			GameEvents.onInputLocksModified.Remove(OnInputLocksChange);
			GameEvents.onKnowledgeChanged.Remove(OnKnowledgeChange);
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

		private void onVesselRename(GameEvents.HostedFromToAction<Vessel, string> nChg)
		{
			if ((Vessel)currentUnowned == nChg.host)
			{
				appFrame.appName.text = currentUnowned.DiscoveryInfo.displayName.Value;
			}
		}

		public void OnPanelHeaderTap(PointerEventData eventData)
		{
			if (InputLockManager.IsUnlocked(ControlTypes.EDITOR_EDIT_NAME_FIELDS) && currentUnowned != null && currentUnowned is Vessel && currentUnowned.DiscoveryInfo.HaveKnowledgeAbout(DiscoveryLevels.Name) && eventData.clickCount == 2 && (currentUnowned as Vessel).vesselType != VesselType.EVA && (currentUnowned as Vessel).vesselType != VesselType.Flag)
			{
				appFrame.appName.text = "|";
				(currentUnowned as Vessel).RenameVessel();
			}
		}

		private void OnInputLocksChange(GameEvents.FromToAction<ControlTypes, ControlTypes> iChg)
		{
			if (InputLockManager.IsUnlocked(ControlTypes.EDITOR_EDIT_NAME_FIELDS) && currentUnowned != null && currentUnowned is Vessel && currentUnowned.DiscoveryInfo.HaveKnowledgeAbout(DiscoveryLevels.Name))
			{
				appFrame.appName.text = currentUnowned.DiscoveryInfo.displayName.Value;
			}
		}

		private void OnKnowledgeChange(GameEvents.HostedFromToAction<IDiscoverable, DiscoveryLevels> kChg)
		{
			if (currentUnowned == kChg.host && kChg.to != DiscoveryLevels.Owned && appFrame != null && appFrame.appName != null && currentUnowned != null)
			{
				appFrame.appName.text = currentUnowned.DiscoveryInfo.displayName.Value;
			}
		}
	}
}
