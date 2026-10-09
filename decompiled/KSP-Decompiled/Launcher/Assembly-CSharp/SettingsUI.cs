using UnityEngine;

public class SettingsUI : MonoBehaviour
{
	public UIButton graphics;

	public UIButton audio;

	public UIButton advanced;

	public UIButton accept;

	public UIButton apply;

	public UIButton cancel;

	public Transform graphicsObjs;

	public Transform audioObjs;

	public Transform advancedObjs;

	public GraphicsScreen gs;

	public AudioScreen ass;

	public CFGFileOperator file;

	public AdvancedScreen adv;

	public AdvancedSettings advS;

	private activeScreen theActiveScreen;

	private bool seenAudio;

	private bool seenAdvanced;

	private void Start()
	{
		advS = GameObject.Find("AdvSettings").GetComponent<AdvancedSettings>();
		file = GameObject.Find("Background").GetComponent<CFGFileOperator>();
		Cursor.SetCursor(null, Vector2.zero, CursorMode.Auto);
		graphics.SetValueChangedDelegate(editorChangedDel);
		audio.SetValueChangedDelegate(editorChangedDel);
		advanced.SetValueChangedDelegate(editorChangedDel);
		accept.SetValueChangedDelegate(editorChangedDel);
		apply.SetValueChangedDelegate(editorChangedDel);
		cancel.SetValueChangedDelegate(editorChangedDel);
		graphicsObjs.gameObject.SetActive(value: true);
		audioObjs.gameObject.SetActive(value: false);
		advancedObjs.gameObject.SetActive(value: false);
		graphics.SetState(2);
		theActiveScreen = activeScreen.GRAPHICS;
		seenAudio = false;
		seenAdvanced = false;
	}

	private void Update()
	{
		switch (theActiveScreen)
		{
		case activeScreen.GRAPHICS:
			graphics.SetState(2);
			audio.SetState(0);
			advanced.SetState(0);
			break;
		case activeScreen.AUDIO:
			graphics.SetState(0);
			audio.SetState(2);
			advanced.SetState(0);
			break;
		case activeScreen.ADVANCED:
			graphics.SetState(0);
			audio.SetState(0);
			advanced.SetState(2);
			break;
		}
	}

	private void writeSettingsFile()
	{
		gs.commitCurrentSetting();
		if (seenAudio)
		{
			ass.commitCurrentSetting();
		}
		if (seenAdvanced)
		{
			adv.commitCurrentSetting();
		}
		file.saveSettings();
		advS.saveAdvancedSettings();
	}

	private void editorChangedDel(IUIObject obj)
	{
		if (obj == graphics)
		{
			graphicsObjs.gameObject.SetActive(value: true);
			audioObjs.gameObject.SetActive(value: false);
			advancedObjs.gameObject.SetActive(value: false);
			theActiveScreen = activeScreen.GRAPHICS;
		}
		else if (obj == audio)
		{
			seenAudio = true;
			graphicsObjs.gameObject.SetActive(value: false);
			audioObjs.gameObject.SetActive(value: true);
			advancedObjs.gameObject.SetActive(value: false);
			theActiveScreen = activeScreen.AUDIO;
		}
		else if (obj == advanced)
		{
			seenAdvanced = true;
			graphicsObjs.gameObject.SetActive(value: false);
			audioObjs.gameObject.SetActive(value: false);
			advancedObjs.gameObject.SetActive(value: true);
			theActiveScreen = activeScreen.ADVANCED;
		}
		else if (obj == cancel)
		{
			Application.LoadLevel(0);
		}
		else if (obj == apply)
		{
			writeSettingsFile();
		}
		else if (obj == accept)
		{
			writeSettingsFile();
			Application.LoadLevel(0);
		}
	}
}
