using System.Diagnostics;
using UnityEngine;
using UnityEngine.UI;

public class MakingHistoryAboutDialog : MonoBehaviour
{
	[SerializeField]
	private RawImage bannerIcon;

	[SerializeField]
	private Button moreInfoButton;

	[SerializeField]
	private Button closeButton;

	[SerializeField]
	private Toggle dontShowAgainToggle;

	public Texture[] banners;

	private bool dontShowAgain;

	private string makingHistoryExpansionURL;

	private void Start()
	{
		Random.InitState((int)KSPUtil.SystemDateTime.DateTimeNow().Ticks);
		int num = Random.Range(0, banners.Length);
		bannerIcon.texture = banners[num];
		moreInfoButton.onClick.AddListener(OnMoreInfoButton);
		closeButton.onClick.AddListener(OnCloseButton);
		dontShowAgainToggle.onValueChanged.AddListener(OnDontShowAgainToggle);
		MenuNavigation.SpawnMenuNavigation(base.gameObject, Navigation.Mode.Automatic);
	}

	private void Update()
	{
		if (Input.GetKeyDown(KeyCode.Escape))
		{
			Dismiss();
		}
	}

	public MakingHistoryAboutDialog Create(string expansionURL)
	{
		MakingHistoryAboutDialog makingHistoryAboutDialog = Object.Instantiate(this);
		makingHistoryAboutDialog.gameObject.SetActive(value: true);
		makingHistoryAboutDialog.transform.position = Vector3.zero;
		makingHistoryAboutDialog.transform.SetParent(DialogCanvasUtil.DialogCanvasRect, worldPositionStays: false);
		makingHistoryAboutDialog.makingHistoryExpansionURL = expansionURL;
		InputLockManager.SetControlLock(ControlTypes.MAIN_MENU, "makingHistoryAboutDialog");
		return makingHistoryAboutDialog;
	}

	protected void Dismiss()
	{
		InputLockManager.RemoveControlLock("makingHistoryAboutDialog");
		if (dontShowAgain)
		{
			GameSettings.MISSION_SHOW_EXPANSION_INFO = !dontShowAgain;
			GameSettings.SaveSettings();
		}
		Object.Destroy(base.gameObject);
	}

	private void OnDontShowAgainToggle(bool value)
	{
		dontShowAgain = value;
	}

	private void OnCloseButton()
	{
		Dismiss();
	}

	private void OnMoreInfoButton()
	{
		Process.Start(makingHistoryExpansionURL);
	}
}
