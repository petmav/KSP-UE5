using UnityEngine;

public class PopupDialogController : MonoBehaviour
{
	public PopupDialog popupDialogBase;

	public Transform popupDialogCanvas;

	public static PopupDialogController Instance { get; private set; }

	public static PopupDialog PopupDialogBase => Instance.popupDialogBase;

	public static Transform PopupDialogCanvas => Instance.popupDialogCanvas;

	private void Awake()
	{
		Instance = this;
	}

	private void OnDestroy()
	{
		if (Instance != null && Instance == this)
		{
			Instance = null;
		}
	}
}
