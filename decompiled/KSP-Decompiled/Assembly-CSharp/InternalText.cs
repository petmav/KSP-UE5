using TMPro;
using UnityEngine;

public class InternalText : MonoBehaviour
{
	public string fontName;

	public float fontSize;

	[HideInInspector]
	public TextMeshPro text;

	private void Awake()
	{
		text = GetComponent<TextMeshPro>();
	}
}
