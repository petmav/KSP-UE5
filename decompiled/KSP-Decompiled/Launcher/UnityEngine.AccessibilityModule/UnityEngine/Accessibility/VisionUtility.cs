#define UNITY_ASSERTIONS
using System;
using System.Linq;
using System.Runtime.CompilerServices;
using UnityEngine.Scripting;

namespace UnityEngine.Accessibility;

[UsedByNativeCode]
public static class VisionUtility
{
	private static readonly Color[] s_ColorBlindSafePalette = new Color[19]
	{
		new Color32(0, 0, 0, byte.MaxValue),
		new Color32(73, 0, 146, byte.MaxValue),
		new Color32(7, 71, 81, byte.MaxValue),
		new Color32(0, 146, 146, byte.MaxValue),
		new Color32(182, 109, byte.MaxValue, byte.MaxValue),
		new Color32(byte.MaxValue, 109, 182, byte.MaxValue),
		new Color32(109, 182, byte.MaxValue, byte.MaxValue),
		new Color32(36, byte.MaxValue, 36, byte.MaxValue),
		new Color32(byte.MaxValue, 182, 219, byte.MaxValue),
		new Color32(182, 219, byte.MaxValue, byte.MaxValue),
		new Color32(byte.MaxValue, byte.MaxValue, 109, byte.MaxValue),
		new Color32(30, 92, 92, byte.MaxValue),
		new Color32(74, 154, 87, byte.MaxValue),
		new Color32(113, 66, 183, byte.MaxValue),
		new Color32(162, 66, 183, byte.MaxValue),
		new Color32(178, 92, 25, byte.MaxValue),
		new Color32(100, 100, 100, byte.MaxValue),
		new Color32(80, 203, 181, byte.MaxValue),
		new Color32(82, 205, 242, byte.MaxValue)
	};

	private static readonly float[] s_ColorBlindSafePaletteLuminanceValues = s_ColorBlindSafePalette.Select((Color c) => ComputePerceivedLuminance(c)).ToArray();

	internal static float ComputePerceivedLuminance(Color color)
	{
		color = color.linear;
		return Mathf.LinearToGammaSpace(0.2126f * color.r + 0.7152f * color.g + 0.0722f * color.b);
	}

	internal static void GetLuminanceValuesForPalette(Color[] palette, ref float[] outLuminanceValues)
	{
		Debug.Assert(palette != null && outLuminanceValues != null, "Passed in arrays can't be null.");
		Debug.Assert(palette.Length == outLuminanceValues.Length, "Passed in arrays need to be of the same length.");
		for (int i = 0; i < palette.Length; i++)
		{
			outLuminanceValues[i] = ComputePerceivedLuminance(palette[i]);
		}
	}

	public unsafe static int GetColorBlindSafePalette(Color[] palette, float minimumLuminance, float maximumLuminance)
	{
		if (palette == null)
		{
			throw new ArgumentNullException("palette");
		}
		fixed (Color* palette2 = palette)
		{
			return GetColorBlindSafePaletteInternal(palette2, palette.Length, minimumLuminance, maximumLuminance, useColor32: false);
		}
	}

	internal unsafe static int GetColorBlindSafePalette(Color32[] palette, float minimumLuminance, float maximumLuminance)
	{
		if (palette == null)
		{
			throw new ArgumentNullException("palette");
		}
		fixed (Color32* palette2 = palette)
		{
			return GetColorBlindSafePaletteInternal(palette2, palette.Length, minimumLuminance, maximumLuminance, useColor32: true);
		}
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	private unsafe static int GetColorBlindSafePaletteInternal(void* palette, int paletteLength, float minimumLuminance, float maximumLuminance, bool useColor32)
	{
		if (palette == null)
		{
			throw new ArgumentNullException("palette");
		}
		Color[] array = (from i in Enumerable.Range(0, s_ColorBlindSafePalette.Length)
			where s_ColorBlindSafePaletteLuminanceValues[i] >= minimumLuminance && s_ColorBlindSafePaletteLuminanceValues[i] <= maximumLuminance
			select s_ColorBlindSafePalette[i]).ToArray();
		int num = Mathf.Min(paletteLength, array.Length);
		if (num > 0)
		{
			for (int j = 0; j < paletteLength; j++)
			{
				if (useColor32)
				{
					*(Color32*)((byte*)palette + (nint)j * (nint)sizeof(Color32)) = array[j % num];
				}
				else
				{
					*(Color*)((byte*)palette + (nint)j * (nint)sizeof(Color)) = array[j % num];
				}
			}
		}
		else
		{
			for (int k = 0; k < paletteLength; k++)
			{
				if (useColor32)
				{
					*(Color32*)((byte*)palette + (nint)k * (nint)sizeof(Color32)) = default(Color32);
				}
				else
				{
					*(Color*)((byte*)palette + (nint)k * (nint)sizeof(Color)) = default(Color);
				}
			}
		}
		return num;
	}
}
