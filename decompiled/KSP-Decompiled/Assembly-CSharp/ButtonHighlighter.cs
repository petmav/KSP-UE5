using UnityEngine;

public class ButtonHighlighter : MonoBehaviour
{
	[SerializeField]
	private GameObject Highlighter;

	public void Enable(bool value)
	{
		Highlighter.SetActive(value);
	}
}
