using System.Diagnostics;
using UnityEngine;
using UnityEngine.UI;

public class SerenityAboutDialog : MonoBehaviour
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

	private string serenityExpansionURL;

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

	public SerenityAboutDialog Create(string expansionURL)
	{
		SerenityAboutDialog serenityAboutDialog = Object.Instantiate(this);
		serenityAboutDialog.gameObject.SetActive(value: true);
		serenityAboutDialog.transform.position = Vector3.zero;
		serenityAboutDialog.transform.SetParent(DialogCanvasUtil.DialogCanvasRect, worldPositionStays: false);
		serenityAboutDialog.serenityExpansionURL = expansionURL;
		InputLockManager.SetControlLock(ControlTypes.MAIN_MENU, "serenityAboutDialog");
		return serenityAboutDialog;
	}

	protected void Dismiss()
	{
		InputLockManager.RemoveControlLock("serenityAboutDialog");
		if (dontShowAgain)
		{
			GameSettings.SERENITY_SHOW_EXPANSION_INFO = !dontShowAgain;
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
		Process.Start(serenityExpansionURL);
	}
}
