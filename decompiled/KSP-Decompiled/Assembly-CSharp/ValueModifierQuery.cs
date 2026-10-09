public class ValueModifierQuery
{
	public enum TextStyling
	{
		None,
		OnGUI,
		EzGUIRichText,
		OnGUI_LessIsGood,
		EzGUIRichText_LessIsGood
	}

	private string valueId;

	private float input;

	private float delta;

	public string ValueId => valueId;

	public ValueModifierQuery(string valueId, float v0)
	{
		this.valueId = valueId;
		input = v0;
		delta = 0f;
	}

	public float GetInput()
	{
		return input;
	}

	public void AddDelta(float val)
	{
		delta += val;
	}

	public float GetEffectDelta()
	{
		return delta;
	}

	public string GetEffectDeltaText(string format, TextStyling textStyle = TextStyling.None)
	{
		string text = "";
		if (delta == 0f)
		{
			return "";
		}
		text = delta.ToString(format);
		switch (textStyle)
		{
		default:
			return ((delta > 0f) ? "(+" : "(") + text + ")";
		case TextStyling.OnGUI:
			return ((delta > 0f) ? "<color=#caff00>(+" : "<color=#feb200>(") + text + ")</color>";
		case TextStyling.EzGUIRichText:
			return ((delta > 0f) ? "<#caff00>(+" : "<#feb200>(") + text + ")</>";
		case TextStyling.OnGUI_LessIsGood:
			return ((delta > 0f) ? "<color=#feb200>(+" : "<color=#caff00>(") + text + ")</color>";
		case TextStyling.EzGUIRichText_LessIsGood:
			return ((delta > 0f) ? "<#feb200>(+" : "<#caff00>(") + text + ")</>";
		}
	}

	public string GetEffectPercentageText(string format, TextStyling textStyle = TextStyling.None)
	{
		string text = "";
		if (delta == 0f)
		{
			return "";
		}
		text = (delta / input * 100f).ToString(format);
		switch (textStyle)
		{
		default:
			return ((delta > 0f) ? "(+" : "(") + text + "%)";
		case TextStyling.OnGUI:
			return ((delta > 0f) ? "<color=#caff00>(+" : "<color=#feb200>(") + text + "%)</color>";
		case TextStyling.EzGUIRichText:
			return ((delta > 0f) ? "<#caff00>(+" : "<#feb200>(") + text + "%)</>";
		case TextStyling.OnGUI_LessIsGood:
			return ((delta > 0f) ? "<color=#feb200>(+" : "<color=#caff00>(") + text + "%)</color>";
		case TextStyling.EzGUIRichText_LessIsGood:
			return ((delta > 0f) ? "<#feb200>(+" : "<#caff00>(") + text + ")%</>";
		}
	}

	public static ValueModifierQuery RunQuery(string valueId, float input)
	{
		ValueModifierQuery valueModifierQuery = new ValueModifierQuery(valueId, input);
		GameEvents.Modifiers.onValueModifierQuery.Fire(valueModifierQuery);
		return valueModifierQuery;
	}
}
