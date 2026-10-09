using System;
using System.Collections.Generic;
using Expansions.Missions;
using KSP.Localization;
using KSP.UI.TooltipTypes;
using TMPro;
using UnityEngine;
using UnityEngine.UI;

namespace KSP.UI.Screens
{
	public class KSCVesselMarker : AnchoredDialog
	{
		public enum DismissAction
		{
			None,
			Fly,
			Recover
		}

		private Vessel v;

		private string vesselName;

		private VesselType vesselType;

		private string situationString;

		private string crewHeaderString;

		private string crewNames;

		private string partString;

		[SerializeField]
		private Button Marker;

		[SerializeField]
		private Image Panel;

		[SerializeField]
		private Button FlyButton;

		[SerializeField]
		private Button RecoverButton;

		[SerializeField]
		private TextMeshProUGUI situationText;

		[SerializeField]
		private TextMeshProUGUI crewHeader;

		[SerializeField]
		private TextMeshProUGUI crewText;

		[SerializeField]
		private TextMeshProUGUI partsText;

		[SerializeField]
		private TextMeshProUGUI typeText;

		[SerializeField]
		private Image vesselIconPanel;

		[SerializeField]
		private Image vesselIconMarker;

		[SerializeField]
		private TextMeshProUGUI MarkerCaptionText;

		[SerializeField]
		private Sprite[] vesselIcons;

		public Callback<Vessel, DismissAction> OnDismiss = delegate
		{
		};

		private bool expanded;

		private bool locked;

		private Coroutine ctrlCleanup;

		private XSelectable[] markerCtrls;

		private XSelectable[] panelCtrls;

		public static KSCVesselMarker Create(Vessel v, Callback<Vessel, DismissAction> onDismiss)
		{
			KSCVesselMarker component = UnityEngine.Object.Instantiate(AssetBase.GetPrefab("KSCVesselMarker")).GetComponent<KSCVesselMarker>();
			component.v = v;
			component.anchor = v.transform;
			component.vesselName = component.makeShipname(v.name);
			component.vesselType = v.vesselType;
			component.situationString = Vessel.GetSituationString(v);
			component.crewHeaderString = Localizer.Format("#autoLOC_900979");
			component.crewNames = "";
			List<ProtoCrewMember> vesselCrew = v.GetVesselCrew();
			int count = vesselCrew.Count;
			for (int i = 0; i < count; i++)
			{
				ProtoCrewMember protoCrewMember = vesselCrew[i];
				if (i == 0)
				{
					component.crewNames += protoCrewMember.name;
					continue;
				}
				if (i <= 3 || v.GetVesselCrew().Count == 4)
				{
					component.crewNames = component.crewNames + "\n" + protoCrewMember.name;
					continue;
				}
				component.crewNames += Localizer.Format("#autoLOC_475646", (v.GetVesselCrew().Count - 4).ToString());
				break;
			}
			component.partString = Localizer.Format("#autoLOC_475652", v.protoVessel.protoPartSnapshots.Count, v.currentStage);
			component.OnDismiss = onDismiss;
			TooltipController_Text component2 = component.RecoverButton.GetComponent<TooltipController_Text>();
			if (HighLogic.CurrentGame.Mode == Game.Modes.MISSION && HighLogic.CurrentGame.Parameters.CustomParams<MissionParamsGeneral>().preventVesselRecovery)
			{
				component.RecoverButton.interactable = false;
				if (component2 != null)
				{
					component2.enabled = true;
					component2.textString = Localizer.Format("#autoLOC_8002108");
					component2.RequireInteractable = false;
				}
			}
			return component;
		}

		protected override void OnPanelSetupComplete()
		{
			markerCtrls = Marker.GetComponentsInChildren<XSelectable>(includeInactive: true);
			panelCtrls = Panel.GetComponentsInChildren<XSelectable>(includeInactive: true);
		}

		private string makeShipname(string originalName)
		{
			string text = originalName;
			if (originalName.Length > 0 && originalName.Contains("("))
			{
				text = originalName.Substring(0, originalName.IndexOf("(", 1, StringComparison.InvariantCulture) - 1);
			}
			if (text.Length > 38)
			{
				text = text.Substring(0, 35) + "...";
			}
			return text;
		}

		public void Expand()
		{
			InputLockManager.SetControlLock(ControlTypes.KSC_FACILITIES, vesselName + "Marker_" + GetInstanceID());
			Marker.gameObject.SetActive(value: false);
			Panel.gameObject.SetActive(value: true);
			SetBgPanel(Panel);
			useOpacityFade = false;
			setOpacity(1f);
			expanded = true;
			StartCoroutine(CallbackUtil.DelayedCallback(1, delegate
			{
				ClearCtrls(markerCtrls);
			}));
		}

		public void Collapse()
		{
			expanded = false;
			InputLockManager.RemoveControlLock(vesselName + "Marker_" + GetInstanceID());
			Marker.gameObject.SetActive(value: true);
			Panel.gameObject.SetActive(value: false);
			SetBgPanel(Marker.targetGraphic);
			useOpacityFade = true;
			StartCoroutine(CallbackUtil.DelayedCallback(1, delegate
			{
				ClearCtrls(panelCtrls);
			}));
		}

		protected override void CreateWindowContent()
		{
			Marker.onClick.AddListener(OnMarkerButtonInput);
			FlyButton.onClick.AddListener(OnFlyButtonInput);
			RecoverButton.onClick.AddListener(OnRecoverButtonInput);
			MarkerCaptionText.text = vesselName;
			if ((int)vesselType > vesselIcons.Length - 1)
			{
				vesselIconMarker.sprite = vesselIcons[2];
				vesselIconPanel.sprite = vesselIcons[2];
			}
			else
			{
				vesselIconMarker.sprite = vesselIcons[(int)vesselType];
				vesselIconPanel.sprite = vesselIcons[(int)vesselType];
			}
			vesselIconMarker.raycastTarget = false;
			vesselIconPanel.raycastTarget = false;
			situationText.text = situationString;
			crewText.text = crewNames;
			crewHeader.text = crewHeaderString;
			typeText.text = Localizer.Format(vesselType.Description());
			partsText.text = partString;
			MarkerCaptionText.gameObject.SetActive(value: false);
			Collapse();
		}

		private void ClearCtrls(XSelectable[] ctrls)
		{
			if (ctrls == null || ctrls.Length == 0)
			{
				return;
			}
			int num = ctrls.Length;
			while (num-- > 0)
			{
				ctrls[num].Clear();
			}
			hover = false;
			int count = uiControls.Count;
			do
			{
				if (count-- <= 0)
				{
					return;
				}
			}
			while (!uiControls[count].Hover);
			hover = true;
		}

		private void OnMarkerButtonInput()
		{
			if (!locked && !expanded)
			{
				Expand();
			}
		}

		private void OnFlyButtonInput()
		{
			Dismiss(DismissAction.Fly);
		}

		private void OnRecoverButtonInput()
		{
			Dismiss(DismissAction.Recover);
		}

		private void onInputLocksModified(GameEvents.FromToAction<ControlTypes, ControlTypes> inputLocks)
		{
			if (InputLockManager.IsLocking(ControlTypes.KSC_UI, inputLocks))
			{
				if (expanded)
				{
					Collapse();
				}
				setOpacity(0.5f);
				base.enabled = false;
				locked = true;
			}
			if (InputLockManager.IsUnlocking(ControlTypes.KSC_UI, inputLocks))
			{
				setOpacity(1f);
				base.enabled = true;
				locked = false;
			}
		}

		protected override string GetWindowTitle()
		{
			return vesselName;
		}

		protected override void OnClickOut()
		{
			if (expanded)
			{
				Dismiss(DismissAction.None);
			}
		}

		public void Dismiss(DismissAction dma)
		{
			Collapse();
			OnDismiss(v, dma);
		}

		protected override void OnLateUpdate()
		{
			if (expanded)
			{
				return;
			}
			if (hover)
			{
				if (!MarkerCaptionText.gameObject.activeSelf)
				{
					MarkerCaptionText.gameObject.SetActive(value: true);
				}
				if (useOpacityFade)
				{
					useOpacityFade = false;
					setOpacity(1f);
				}
			}
			else
			{
				if (MarkerCaptionText.gameObject.activeSelf)
				{
					MarkerCaptionText.gameObject.SetActive(value: false);
				}
				if (!useOpacityFade)
				{
					useOpacityFade = true;
					lastOpacity = 1f;
				}
			}
		}

		protected override void StartThis()
		{
			GameEvents.onInputLocksModified.Add(onInputLocksModified);
		}

		protected override void OnDestroyThis()
		{
			GameEvents.onInputLocksModified.Remove(onInputLocksModified);
		}
	}
}
