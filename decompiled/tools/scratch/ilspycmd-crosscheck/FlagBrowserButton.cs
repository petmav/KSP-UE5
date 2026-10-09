using UnityEngine;
using UnityEngine.UI;

public class FlagBrowserButton : MonoBehaviour
{
	public FlagBrowser flagBrowserPrefab;

	public RawImage flagRawImage;

	public Button button;

	internal FlagBrowser browser;

	private FlagBrowser.FlagSelectedCallback OnFlagSelected;

	private Callback OnFlagCancelled;

	internal Callback<string> OnFlagSelectedURL;

	internal bool isFlagDecalBrowsing;

	private void Start()
	{
		button.onClick.AddListener(SpawnBrowser);
	}

	public void Setup(Texture texture, Callback onBrowserOpen, FlagBrowser.FlagSelectedCallback onFlagSelect, Callback onFlagCancel)
	{
		SetFlag(texture);
		OnFlagSelected = onFlagSelect;
		OnFlagCancelled = onFlagCancel;
	}

	public void SetFlag(Texture texture)
	{
		flagRawImage.texture = texture;
	}

	internal void SpawnBrowser()
	{
		browser = Object.Instantiate(flagBrowserPrefab);
		browser.OnDismiss = OnFlagCancel;
		browser.OnFlagSelected = OnFlagSelect;
		InputLockManager.SetControlLock(ControlTypes.EDITOR_SOFT_LOCK, "FlagBrowser");
		button.interactable = false;
	}

	private void OnFlagSelect(FlagBrowser.FlagEntry selected)
	{
		if (isFlagDecalBrowsing)
		{
			isFlagDecalBrowsing = false;
			if (OnFlagSelectedURL != null)
			{
				OnFlagSelectedURL(selected.textureInfo.name);
			}
			InputLockManager.RemoveControlLock("FlagBrowser");
			button.interactable = true;
			if (browser != null && browser.OnDismiss != null)
			{
				browser.OnDismiss();
			}
		}
		else
		{
			if (HighLogic.CurrentGame != null)
			{
				HighLogic.CurrentGame.flagURL = selected.textureInfo.name;
				GameEvents.onFlagSelect.Fire(HighLogic.CurrentGame.flagURL);
			}
			GameEvents.onFlagSelect.Fire(selected.textureInfo.name);
			InputLockManager.RemoveControlLock("FlagBrowser");
			button.interactable = true;
			SetFlag(selected.textureInfo.texture);
			OnFlagSelected(selected);
		}
	}

	private void OnFlagCancel()
	{
		InputLockManager.RemoveControlLock("FlagBrowser");
		button.interactable = true;
		OnFlagCancelled();
	}
}
