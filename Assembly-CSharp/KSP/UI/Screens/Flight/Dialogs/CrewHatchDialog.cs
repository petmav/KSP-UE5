using System.Collections.Generic;
using KSP.Localization;
using TMPro;
using UnityEngine;

namespace KSP.UI.Screens.Flight.Dialogs
{
	public class CrewHatchDialog : MonoBehaviour
	{
		[SerializeField]
		private TextMeshProUGUI textHeader;

		[SerializeField]
		private TextMeshProUGUI textModuleCrew;

		[SerializeField]
		private GameObject widgetPrefab;

		[SerializeField]
		private GameObject emptyModuleObject;

		[SerializeField]
		private TextMeshProUGUI emptyModuleText;

		[SerializeField]
		private Transform listContainer;

		[SerializeField]
		private XSelectable hoverArea;

		private bool showTransfer;

		private bool showEVA;

		private Callback<ProtoCrewMember> onEVAEvtHandler;

		private Callback<ProtoCrewMember> onTransferEvtHandler;

		private Callback onDestroyEvtHandler;

		private Part part;

		private List<CrewHatchDialogWidget> widgets;

		private bool hover => hoverArea.Hover;

		public Part Part => part;

		public static CrewHatchDialog Spawn(Part p, bool showEVA, bool showTransfer, Callback<ProtoCrewMember> onEVAEvtHandler, Callback<ProtoCrewMember> onTransferEvtHandler, Callback onDismiss)
		{
			CrewHatchDialog component = Object.Instantiate(AssetBase.GetPrefab("CrewHatchDialog")).GetComponent<CrewHatchDialog>();
			(component.transform as RectTransform).SetParent(DialogCanvasUtil.DialogCanvasRect, worldPositionStays: false);
			component.part = p;
			component.showEVA = showEVA;
			component.showTransfer = showTransfer;
			component.onEVAEvtHandler = onEVAEvtHandler;
			component.onTransferEvtHandler = onTransferEvtHandler;
			component.onDestroyEvtHandler = onDismiss;
			return component;
		}

		public void Terminate()
		{
			if (onDestroyEvtHandler != null)
			{
				onDestroyEvtHandler();
			}
			Object.Destroy(base.gameObject);
		}

		protected void Awake()
		{
			widgets = new List<CrewHatchDialogWidget>();
		}

		protected void Start()
		{
			CreatePanelContent();
			GameEvents.onVesselChange.Add(onVesselChange);
			GameEvents.onVesselWasModified.Add(onVesselWasModified);
			GameEvents.OnFlightUIModeChanged.Add(OnFlightUIModeChanged);
			GameEvents.OnCameraChange.Add(OnCameraChange);
		}

		protected void OnDestroy()
		{
			GameEvents.OnCameraChange.Remove(OnCameraChange);
			GameEvents.OnFlightUIModeChanged.Remove(OnFlightUIModeChanged);
			GameEvents.onVesselWasModified.Remove(onVesselWasModified);
			GameEvents.onVesselChange.Remove(onVesselChange);
		}

		protected void OnCameraChange(CameraManager.CameraMode data)
		{
			Terminate();
		}

		protected void OnFlightUIModeChanged(FlightUIMode data)
		{
			Terminate();
		}

		protected void onVesselWasModified(Vessel data)
		{
			Terminate();
		}

		protected void onVesselChange(Vessel data)
		{
			Terminate();
		}

		protected void CreatePanelContent()
		{
			textHeader.text = Localizer.Format("#autoLOC_900979", part.partInfo.title);
			Debug.Log("Part Name: " + part.partInfo.title);
			textModuleCrew.text = Localizer.Format("#autoLOC_6002258", part.protoModuleCrew.Count, part.CrewCapacity);
			emptyModuleText.text = Localizer.Format("#autoLOC_6002404");
			CreateList(part.protoModuleCrew);
		}

		protected void ClearList()
		{
			int count = widgets.Count;
			while (count-- > 0)
			{
				widgets[count].Terminate();
			}
			widgets.Clear();
		}

		protected void CreateList(List<ProtoCrewMember> crew)
		{
			ClearList();
			UIAvailability transferBtnAvail = ((!showTransfer) ? UIAvailability.Hidden : UIAvailability.Available);
			UIAvailability evaBtnAvail = (showEVA ? UIAvailability.GreyedOut : UIAvailability.Hidden);
			int i = 0;
			for (int count = crew.Count; i < count; i++)
			{
				if (!crew[i].inactive)
				{
					if (showEVA && HighLogic.CurrentGame.Parameters.Flight.CanEVA && crew[i].type != ProtoCrewMember.KerbalType.Tourist)
					{
						evaBtnAvail = UIAvailability.Available;
					}
					CrewHatchDialogWidget component = Object.Instantiate(widgetPrefab).GetComponent<CrewHatchDialogWidget>();
					component.Init(crew[i], OnBtnEVA, OnBtnTransfer, evaBtnAvail, transferBtnAvail);
					component.transform.SetParent(listContainer, worldPositionStays: false);
					widgets.Add(component);
				}
			}
			emptyModuleObject.SetActive(widgets.Count == 0);
		}

		protected void LateUpdate()
		{
			if (!hover && (Mouse.Left.GetButtonDown() || Mouse.Right.GetButtonDown()))
			{
				Terminate();
			}
		}

		protected void OnBtnEVA(ProtoCrewMember crew)
		{
			if (onEVAEvtHandler != null)
			{
				onEVAEvtHandler(crew);
			}
			Terminate();
		}

		protected void OnBtnTransfer(ProtoCrewMember crew)
		{
			if (onTransferEvtHandler != null)
			{
				onTransferEvtHandler(crew);
			}
			Terminate();
		}
	}
}
