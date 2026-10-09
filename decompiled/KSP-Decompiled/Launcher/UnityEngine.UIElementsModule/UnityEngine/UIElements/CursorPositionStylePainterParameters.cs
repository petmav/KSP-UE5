namespace UnityEngine.UIElements;

internal struct CursorPositionStylePainterParameters
{
	public Rect rect;

	public string text;

	public Font font;

	public int fontSize;

	public FontStyle fontStyle;

	public TextAnchor anchor;

	public float wordWrapWidth;

	public bool richText;

	public int cursorIndex;

	public static CursorPositionStylePainterParameters GetDefault(VisualElement ve, string text)
	{
		ComputedStyle computedStyle = ve.computedStyle;
		CursorPositionStylePainterParameters result = default(CursorPositionStylePainterParameters);
		result.rect = ve.contentRect;
		result.text = text;
		result.font = computedStyle.unityFont.value;
		result.fontSize = (int)computedStyle.fontSize.value.value;
		result.fontStyle = computedStyle.unityFontStyleAndWeight.value;
		result.anchor = computedStyle.unityTextAlign.value;
		result.wordWrapWidth = ((computedStyle.whiteSpace.value == WhiteSpace.Normal) ? ve.contentRect.width : 0f);
		result.richText = false;
		result.cursorIndex = 0;
		return result;
	}

	internal TextNativeSettings GetTextNativeSettings(float scaling)
	{
		TextNativeSettings result = default(TextNativeSettings);
		result.text = text;
		result.font = font;
		result.size = fontSize;
		result.scaling = scaling;
		result.style = fontStyle;
		result.color = Color.white;
		result.anchor = anchor;
		result.wordWrap = true;
		result.wordWrapWidth = wordWrapWidth;
		result.richText = richText;
		return result;
	}
}
