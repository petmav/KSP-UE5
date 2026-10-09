using System;

namespace UnityEngine.UIElements;

internal static class MeshGenerationContextUtils
{
	public struct BorderParams
	{
		public Rect rect;

		public Color playmodeTintColor;

		public Color leftColor;

		public Color topColor;

		public Color rightColor;

		public Color bottomColor;

		public float leftWidth;

		public float topWidth;

		public float rightWidth;

		public float bottomWidth;

		public Vector2 topLeftRadius;

		public Vector2 topRightRadius;

		public Vector2 bottomRightRadius;

		public Vector2 bottomLeftRadius;

		public Material material;
	}

	public struct RectangleParams
	{
		public Rect rect;

		public Rect uv;

		public Color color;

		public Texture texture;

		public VectorImage vectorImage;

		public Material material;

		public ScaleMode scaleMode;

		public Color playmodeTintColor;

		public Vector2 topLeftRadius;

		public Vector2 topRightRadius;

		public Vector2 bottomRightRadius;

		public Vector2 bottomLeftRadius;

		public int leftSlice;

		public int topSlice;

		public int rightSlice;

		public int bottomSlice;

		public static RectangleParams MakeSolid(Rect rect, Color color, ContextType panelContext)
		{
			Color color2 = ((panelContext == ContextType.Editor) ? UIElementsUtility.editorPlayModeTintColor : Color.white);
			RectangleParams result = default(RectangleParams);
			result.rect = rect;
			result.color = color;
			result.uv = new Rect(0f, 0f, 1f, 1f);
			result.playmodeTintColor = color2;
			return result;
		}

		public static RectangleParams MakeTextured(Rect rect, Rect uv, Texture texture, ScaleMode scaleMode, ContextType panelContext)
		{
			Color color = ((panelContext == ContextType.Editor) ? UIElementsUtility.editorPlayModeTintColor : Color.white);
			float num = (float)texture.width * uv.width / ((float)texture.height * uv.height);
			float num2 = rect.width / rect.height;
			switch (scaleMode)
			{
			case ScaleMode.ScaleAndCrop:
				if (num2 > num)
				{
					float num5 = uv.height * (num / num2);
					float num6 = (uv.height - num5) * 0.5f;
					uv = new Rect(uv.x, uv.y + num6, uv.width, num5);
				}
				else
				{
					float num7 = uv.width * (num2 / num);
					float num8 = (uv.width - num7) * 0.5f;
					uv = new Rect(uv.x + num8, uv.y, num7, uv.height);
				}
				break;
			case ScaleMode.ScaleToFit:
				if (num2 > num)
				{
					float num3 = num / num2;
					rect = new Rect(rect.xMin + rect.width * (1f - num3) * 0.5f, rect.yMin, num3 * rect.width, rect.height);
				}
				else
				{
					float num4 = num2 / num;
					rect = new Rect(rect.xMin, rect.yMin + rect.height * (1f - num4) * 0.5f, rect.width, num4 * rect.height);
				}
				break;
			default:
				throw new NotImplementedException();
			case ScaleMode.StretchToFill:
				break;
			}
			RectangleParams result = default(RectangleParams);
			result.rect = rect;
			result.uv = uv;
			result.color = Color.white;
			result.texture = texture;
			result.scaleMode = scaleMode;
			result.playmodeTintColor = color;
			return result;
		}

		public static RectangleParams MakeVectorTextured(Rect rect, Rect uv, VectorImage vectorImage, ScaleMode scaleMode, ContextType panelContext)
		{
			Color color = ((panelContext == ContextType.Editor) ? UIElementsUtility.editorPlayModeTintColor : Color.white);
			RectangleParams result = default(RectangleParams);
			result.rect = rect;
			result.uv = uv;
			result.color = Color.white;
			result.vectorImage = vectorImage;
			result.scaleMode = scaleMode;
			result.playmodeTintColor = color;
			return result;
		}

		internal bool HasRadius(float epsilon)
		{
			return (topLeftRadius.x > epsilon && topLeftRadius.y > epsilon) || (topRightRadius.x > epsilon && topRightRadius.y > epsilon) || (bottomRightRadius.x > epsilon && bottomRightRadius.y > epsilon) || (bottomLeftRadius.x > epsilon && bottomLeftRadius.y > epsilon);
		}
	}

	public struct TextParams
	{
		public Rect rect;

		public string text;

		public Font font;

		public int fontSize;

		public FontStyle fontStyle;

		public Color fontColor;

		public TextAnchor anchor;

		public bool wordWrap;

		public float wordWrapWidth;

		public bool richText;

		public Material material;

		public Color playmodeTintColor;

		public override int GetHashCode()
		{
			int hashCode = rect.GetHashCode();
			hashCode = (hashCode * 397) ^ ((text != null) ? text.GetHashCode() : 0);
			hashCode = (hashCode * 397) ^ ((font != null) ? font.GetHashCode() : 0);
			hashCode = (hashCode * 397) ^ fontSize;
			hashCode = (hashCode * 397) ^ (int)fontStyle;
			hashCode = (hashCode * 397) ^ fontColor.GetHashCode();
			hashCode = (hashCode * 397) ^ (int)anchor;
			hashCode = (hashCode * 397) ^ wordWrap.GetHashCode();
			hashCode = (hashCode * 397) ^ wordWrapWidth.GetHashCode();
			hashCode = (hashCode * 397) ^ richText.GetHashCode();
			hashCode = (hashCode * 397) ^ ((material != null) ? material.GetHashCode() : 0);
			return (hashCode * 397) ^ playmodeTintColor.GetHashCode();
		}

		internal static TextParams MakeStyleBased(VisualElement ve, string text)
		{
			ComputedStyle computedStyle = ve.computedStyle;
			TextParams result = default(TextParams);
			result.rect = ve.contentRect;
			result.text = text;
			result.font = computedStyle.unityFont.value;
			result.fontSize = (int)computedStyle.fontSize.value.value;
			result.fontStyle = computedStyle.unityFontStyleAndWeight.value;
			result.fontColor = computedStyle.color.value;
			result.anchor = computedStyle.unityTextAlign.value;
			result.wordWrap = computedStyle.whiteSpace.value == WhiteSpace.Normal;
			result.wordWrapWidth = ((computedStyle.whiteSpace.value == WhiteSpace.Normal) ? ve.contentRect.width : 0f);
			result.richText = false;
			IPanel panel = ve.panel;
			result.playmodeTintColor = ((panel != null && panel.contextType == ContextType.Editor) ? UIElementsUtility.editorPlayModeTintColor : Color.white);
			return result;
		}

		internal static TextNativeSettings GetTextNativeSettings(TextParams textParams, float scaling)
		{
			TextNativeSettings result = default(TextNativeSettings);
			result.text = textParams.text;
			result.font = textParams.font;
			result.size = textParams.fontSize;
			result.scaling = scaling;
			result.style = textParams.fontStyle;
			result.color = textParams.fontColor;
			result.anchor = textParams.anchor;
			result.wordWrap = textParams.wordWrap;
			result.wordWrapWidth = textParams.wordWrapWidth;
			result.richText = textParams.richText;
			return result;
		}
	}

	public static void Rectangle(this MeshGenerationContext mgc, RectangleParams rectParams)
	{
		mgc.painter.DrawRectangle(rectParams);
	}

	public static void Border(this MeshGenerationContext mgc, BorderParams borderParams)
	{
		mgc.painter.DrawBorder(borderParams);
	}

	public static void Text(this MeshGenerationContext mgc, TextParams textParams, TextHandle handle, float pixelsPerPoint)
	{
		if (textParams.font != null)
		{
			mgc.painter.DrawText(textParams, handle, pixelsPerPoint);
		}
	}

	private static Vector2 ConvertBorderRadiusPercentToPoints(Vector2 borderRectSize, Length length)
	{
		float a = length.value;
		float a2 = length.value;
		if (length.unit == LengthUnit.Percent)
		{
			a = borderRectSize.x * length.value / 100f;
			a2 = borderRectSize.y * length.value / 100f;
		}
		a = Mathf.Max(a, 0f);
		a2 = Mathf.Max(a2, 0f);
		return new Vector2(a, a2);
	}

	public static void GetVisualElementRadii(VisualElement ve, out Vector2 topLeft, out Vector2 bottomLeft, out Vector2 topRight, out Vector2 bottomRight)
	{
		IResolvedStyle resolvedStyle = ve.resolvedStyle;
		Vector2 borderRectSize = new Vector2(resolvedStyle.width, resolvedStyle.height);
		ComputedStyle computedStyle = ve.computedStyle;
		topLeft = ConvertBorderRadiusPercentToPoints(borderRectSize, computedStyle.borderTopLeftRadius.value);
		bottomLeft = ConvertBorderRadiusPercentToPoints(borderRectSize, computedStyle.borderBottomLeftRadius.value);
		topRight = ConvertBorderRadiusPercentToPoints(borderRectSize, computedStyle.borderTopRightRadius.value);
		bottomRight = ConvertBorderRadiusPercentToPoints(borderRectSize, computedStyle.borderBottomRightRadius.value);
	}
}
