using TMPro;
using UnityEngine;
using UnityEngine.UI;

public static class TMPProUtil
{
	public static TextAlignmentOptions TextAlignment(TextAnchor anchor)
	{
		switch (anchor)
		{
		default:
			return TextAlignmentOptions.Left;
		case TextAnchor.UpperLeft:
			return TextAlignmentOptions.TopLeft;
		case TextAnchor.UpperCenter:
			return TextAlignmentOptions.Top;
		case TextAnchor.UpperRight:
			return TextAlignmentOptions.TopRight;
		case TextAnchor.MiddleLeft:
			return TextAlignmentOptions.MidlineLeft;
		case TextAnchor.MiddleCenter:
			return TextAlignmentOptions.Center;
		case TextAnchor.MiddleRight:
			return TextAlignmentOptions.MidlineRight;
		case TextAnchor.LowerLeft:
			return TextAlignmentOptions.BottomLeft;
		case TextAnchor.LowerCenter:
			return TextAlignmentOptions.Bottom;
		case TextAnchor.LowerRight:
			return TextAlignmentOptions.BottomRight;
		}
	}

	public static Component GetComponentByInstanceID(this GameObject go, int objectID)
	{
		Component[] componentsInChildren = go.GetComponentsInChildren<Component>();
		int num = 0;
		while (true)
		{
			if (num < componentsInChildren.Length)
			{
				if (componentsInChildren[num].GetInstanceID() == objectID)
				{
					break;
				}
				num++;
				continue;
			}
			return null;
		}
		return componentsInChildren[num];
	}

	public static FontStyles FontStyle(FontStyle style)
	{
		switch (style)
		{
		default:
			return FontStyles.Normal;
		case UnityEngine.FontStyle.Normal:
			return FontStyles.Normal;
		case UnityEngine.FontStyle.Bold:
			return FontStyles.Bold;
		case UnityEngine.FontStyle.Italic:
			return FontStyles.Italic;
		case UnityEngine.FontStyle.BoldAndItalic:
			return FontStyles.Italic;
		}
	}

	public static TMP_InputField.LineType LineType(InputField.LineType type)
	{
		switch (type)
		{
		case InputField.LineType.SingleLine:
			return TMP_InputField.LineType.SingleLine;
		case InputField.LineType.MultiLineSubmit:
			return TMP_InputField.LineType.MultiLineSubmit;
		default:
			return TMP_InputField.LineType.MultiLineNewline;
		}
	}
}
