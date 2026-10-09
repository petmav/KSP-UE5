using System.Collections.Generic;
using Lingoona;
using UnityEngine;
using UnityEngine.UI;

public class LingoonaGrammarFormatter : MonoBehaviour
{
	private Text text;

	[SerializeField]
	public string Template;

	[SerializeField]
	public List<string> Parameters;

	private void Start()
	{
		text = GetComponent<Text>();
	}

	private void Update()
	{
		if ((bool)text)
		{
			text.text = Grammar.useGrammar(Template, Parameters);
		}
	}
}
