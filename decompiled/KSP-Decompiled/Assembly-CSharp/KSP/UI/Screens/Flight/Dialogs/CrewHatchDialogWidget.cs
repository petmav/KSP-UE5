using System;
using TMPro;
using UnityEngine;
using UnityEngine.Events;
using UnityEngine.UI;

namespace KSP.UI.Screens.Flight.Dialogs
{
	public class CrewHatchDialogWidget : MonoBehaviour
	{
		[SerializeField]
		private TextMeshProUGUI textCrewName;

		[SerializeField]
		private Button btnEVA;

		[SerializeField]
		private Button btnTransfer;

		[NonSerialized]
		public ProtoCrewMember protoCrewMember;

		public void Init(ProtoCrewMember crew, Callback<ProtoCrewMember> onBtnEVA, Callback<ProtoCrewMember> onBtnTransfer, UIAvailability evaBtnAvail, UIAvailability transferBtnAvail)
		{
			protoCrewMember = crew;
			textCrewName.text = crew.name;
			BtnInit(btnEVA, evaBtnAvail, delegate
			{
				onBtnEVA(crew);
			});
			BtnInit(btnTransfer, transferBtnAvail, delegate
			{
				onBtnTransfer(crew);
			});
		}

		protected void BtnInit(Button btn, UIAvailability btnAvail, UnityAction btnEvtHandler)
		{
			switch (btnAvail)
			{
			case UIAvailability.Available:
				btn.onClick.AddListener(btnEvtHandler);
				break;
			case UIAvailability.GreyedOut:
				btn.interactable = false;
				break;
			case UIAvailability.Hidden:
				btn.gameObject.SetActive(value: false);
				break;
			}
		}

		public void Terminate()
		{
			UnityEngine.Object.Destroy(base.gameObject);
		}
	}
}
