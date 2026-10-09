using System.Diagnostics;
using System.Runtime.InteropServices;

namespace UnityEngine.UIElements.StyleSheets;

[StructLayout(LayoutKind.Explicit)]
[DebuggerDisplay("id = {id}, keyword = {keyword}, number = {number}, boolean = {boolean}, color = {color}, resource = {resource}")]
internal struct StyleValue
{
	[FieldOffset(0)]
	public StylePropertyID id;

	[FieldOffset(4)]
	public StyleKeyword keyword;

	[FieldOffset(8)]
	public float number;

	[FieldOffset(8)]
	public Length length;

	[FieldOffset(8)]
	public Color color;

	[FieldOffset(8)]
	public GCHandle resource;

	public static StyleValue Create(StylePropertyID id)
	{
		StyleValue result = default(StyleValue);
		result.id = id;
		return result;
	}

	public static StyleValue Create(StylePropertyID id, StyleKeyword keyword)
	{
		StyleValue result = default(StyleValue);
		result.id = id;
		result.keyword = keyword;
		return result;
	}

	public static StyleValue Create(StylePropertyID id, float number)
	{
		StyleValue result = default(StyleValue);
		result.id = id;
		result.number = number;
		return result;
	}

	public static StyleValue Create(StylePropertyID id, int number)
	{
		StyleValue result = default(StyleValue);
		result.id = id;
		result.number = number;
		return result;
	}

	public static StyleValue Create(StylePropertyID id, Color color)
	{
		StyleValue result = default(StyleValue);
		result.id = id;
		result.color = color;
		return result;
	}
}
