using System.Collections.Generic;
using KSP.Localization;
using UnityEngine;
using UnityEngine.UI;

public class MiniSettings : MonoBehaviour
{
	private Callback OnDismissCallback;

	private Vector2 scrollPos;

	private Rect windowRect;

	private UISkinDef skin;

	private PopupDialog popupDialog;

	private AudioFXSettings audioSettings;

	private VideoSettings videoSettings;

	private GameplaySettingsScreen gameSettings;

	private AlarmClockSettingsUI alarmSettings;

	private MenuNavigation menuNav;

	private ScrollRect scrollRect;

	private RectTransform content;

	private GameObject tempMiniSettingsObj;

	private Transform explicitNavParentcache;

	private List<Selectable> explicitNavSelectables = new List<Selectable>();

	public static MiniSettings Create(Callback onDismiss)
	{
		MiniSettings miniSettings = new GameObject("Mini Settings Dialog").AddComponent<MiniSettings>();
		miniSettings.OnDismissCallback = onDismiss;
		miniSettings.windowRect = new Rect(0.5f, 0.5f, 400f, 460f);
		miniSettings.skin = UISkinManager.GetSkin("MiniSettingsSkin");
		miniSettings.audioSettings = new AudioFXSettings();
		miniSettings.videoSettings = new VideoSettings();
		miniSettings.gameSettings = new GameplaySettingsScreen();
		miniSettings.alarmSettings = new AlarmClockSettingsUI();
		return miniSettings;
	}

	private void Start()
	{
		audioSettings.GetSettings();
		videoSettings.GetSettings();
		gameSettings.GetSettings();
		alarmSettings.GetSettings();
		popupDialog = PopupDialog.SpawnPopupDialog(new MultiOptionDialog("Settings", "", Localizer.Format("#autoLOC_149458"), skin, windowRect, drawWindow()), persistAcrossScenes: false, skin);
		popupDialog.OnDismiss = Dismiss;
		menuNav = MenuNavigation.SpawnMenuNavigation(popupDialog.gameObject, Navigation.Mode.Vertical, hasText: true, limitCheck: true, SliderFocusType.AnchoredPos);
		menuNav.SetVerticalExplicitNavigation(menuNav.selectableItems);
		menuNav.SetSameParentExplicitNavigation(menuNav.selectableItems);
		tempMiniSettingsObj = popupDialog.gameObject;
	}

	private void Update()
	{
		audioSettings.OnUpdate();
		videoSettings.OnUpdate();
		gameSettings.OnUpdate();
		alarmSettings.OnUpdate();
	}

	private DialogGUIBase[] drawWindow()
	{
		List<DialogGUIBase> list = new List<DialogGUIBase>();
		DialogGUIVerticalLayout dialogGUIVerticalLayout = new DialogGUIVerticalLayout(-1f, 450f, 4f, new RectOffset(8, 24, 16, 16), TextAnchor.UpperLeft, new DialogGUIContentSizer(ContentSizeFitter.FitMode.Unconstrained, ContentSizeFitter.FitMode.PreferredSize, useParentSize: true), new DialogGUIHorizontalLayout(new DialogGUIFlexibleSpace(), new DialogGUIButton(() => Localizer.Format("#autoLOC_149478", GameParameters.GetPresetColorHex(HighLogic.CurrentGame.Parameters.preset), HighLogic.CurrentGame.Parameters.preset.Description()), delegate
		{
			OpenDifficultyOptions();
		}, () => true, 180f, 30f, dismissOnSelect: false, skin.customStyles[0]), new DialogGUIFlexibleSpace()));
		DialogGUIScrollList item = new DialogGUIScrollList(-Vector2.one, hScroll: false, vScroll: true, dialogGUIVerticalLayout);
		list.Add(item);
		dialogGUIVerticalLayout.AddChildren(audioSettings.DrawMiniSettings());
		dialogGUIVerticalLayout.AddChildren(videoSettings.DrawMiniSettings());
		dialogGUIVerticalLayout.AddChildren(gameSettings.DrawMiniSettings());
		dialogGUIVerticalLayout.AddChildren(alarmSettings.DrawMiniSettings());
		list.Add(new DialogGUIHorizontalLayout(new DialogGUIFlexibleSpace(), new DialogGUIButton(Localizer.Format("#autoLOC_149512"), delegate
		{
			ApplySettings();
		}, 80f, 30f, false), new DialogGUIButton(Localizer.Format("#autoLOC_149513"), delegate
		{
			ApplySettings();
			Dismiss();
		}, 80f, 30f, true), new DialogGUIButton(Localizer.Format("#autoLOC_149514"), delegate
		{
			Dismiss();
		}, 80f, 30f, true)));
		list.Add(new DialogGUISpace(4f));
		list.Add(new DialogGUIHorizontalLayout(new DialogGUIFlexibleSpace(), new DialogGUILabel($"<color=#d0d0d0><i>{Versioning.GetVersionStringFull()}</i></color>")));
		return list.ToArray();
	}

	private void OpenDifficultyOptions()
	{
		DifficultyOptionsMenu.Create(HighLogic.CurrentGame.Mode, HighLogic.CurrentGame.Parameters, newGame: false, OnDifficultyOptionsDismiss, tempMiniSettingsObj);
		tempMiniSettingsObj.SetActive(value: false);
	}

	private void OnDifficultyOptionsDismiss(GameParameters pars, bool changed)
	{
		HighLogic.CurrentGame.Parameters = pars;
	}

	private void ApplySettings()
	{
		audioSettings.ApplySettings();
		videoSettings.ApplySettings();
		gameSettings.ApplySettings();
		alarmSettings.ApplySettings();
		GameSettings.SaveSettings();
		GameEvents.OnGameSettingsApplied.Fire();
	}

	public void Dismiss()
	{
		gameSettings.ApplyUIScalingAndAdjustments();
		OnDismissCallback();
		Object.Destroy(base.gameObject);
	}
}
