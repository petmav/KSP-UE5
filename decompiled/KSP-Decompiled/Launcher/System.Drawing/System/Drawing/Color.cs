namespace System.Drawing;

[Serializable]
public struct Color
{
	[Flags]
	internal enum ColorType : short
	{
		Empty = 0,
		Known = 1,
		ARGB = 2,
		Named = 4,
		System = 8
	}

	private long value;

	internal short state;

	internal short knownColor;

	internal string name;

	public static readonly Color Empty;

	public string Name
	{
		get
		{
			if (name == null)
			{
				if (IsNamedColor)
				{
					name = KnownColors.GetName(knownColor);
				}
				else
				{
					name = $"{ToArgb():x}";
				}
			}
			return name;
		}
	}

	public bool IsKnownColor => (state & 1) != 0;

	public bool IsSystemColor => (state & 8) != 0;

	public bool IsNamedColor => (state & 5) != 0;

	internal long Value
	{
		get
		{
			if (value == 0L && IsKnownColor)
			{
				value = FromKnownColor((KnownColor)knownColor).ToArgb() & 0xFFFFFFFFu;
			}
			return value;
		}
		set
		{
			this.value = value;
		}
	}

	public bool IsEmpty => state == 0;

	public byte A => (byte)(Value >> 24);

	public byte R => (byte)(Value >> 16);

	public byte G => (byte)(Value >> 8);

	public byte B => (byte)Value;

	public static Color Transparent => FromKnownColor(KnownColor.Transparent);

	public static Color AliceBlue => FromKnownColor(KnownColor.AliceBlue);

	public static Color AntiqueWhite => FromKnownColor(KnownColor.AntiqueWhite);

	public static Color Aqua => FromKnownColor(KnownColor.Aqua);

	public static Color Aquamarine => FromKnownColor(KnownColor.Aquamarine);

	public static Color Azure => FromKnownColor(KnownColor.Azure);

	public static Color Beige => FromKnownColor(KnownColor.Beige);

	public static Color Bisque => FromKnownColor(KnownColor.Bisque);

	public static Color Black => FromKnownColor(KnownColor.Black);

	public static Color BlanchedAlmond => FromKnownColor(KnownColor.BlanchedAlmond);

	public static Color Blue => FromKnownColor(KnownColor.Blue);

	public static Color BlueViolet => FromKnownColor(KnownColor.BlueViolet);

	public static Color Brown => FromKnownColor(KnownColor.Brown);

	public static Color BurlyWood => FromKnownColor(KnownColor.BurlyWood);

	public static Color CadetBlue => FromKnownColor(KnownColor.CadetBlue);

	public static Color Chartreuse => FromKnownColor(KnownColor.Chartreuse);

	public static Color Chocolate => FromKnownColor(KnownColor.Chocolate);

	public static Color Coral => FromKnownColor(KnownColor.Coral);

	public static Color CornflowerBlue => FromKnownColor(KnownColor.CornflowerBlue);

	public static Color Cornsilk => FromKnownColor(KnownColor.Cornsilk);

	public static Color Crimson => FromKnownColor(KnownColor.Crimson);

	public static Color Cyan => FromKnownColor(KnownColor.Cyan);

	public static Color DarkBlue => FromKnownColor(KnownColor.DarkBlue);

	public static Color DarkCyan => FromKnownColor(KnownColor.DarkCyan);

	public static Color DarkGoldenrod => FromKnownColor(KnownColor.DarkGoldenrod);

	public static Color DarkGray => FromKnownColor(KnownColor.DarkGray);

	public static Color DarkGreen => FromKnownColor(KnownColor.DarkGreen);

	public static Color DarkKhaki => FromKnownColor(KnownColor.DarkKhaki);

	public static Color DarkMagenta => FromKnownColor(KnownColor.DarkMagenta);

	public static Color DarkOliveGreen => FromKnownColor(KnownColor.DarkOliveGreen);

	public static Color DarkOrange => FromKnownColor(KnownColor.DarkOrange);

	public static Color DarkOrchid => FromKnownColor(KnownColor.DarkOrchid);

	public static Color DarkRed => FromKnownColor(KnownColor.DarkRed);

	public static Color DarkSalmon => FromKnownColor(KnownColor.DarkSalmon);

	public static Color DarkSeaGreen => FromKnownColor(KnownColor.DarkSeaGreen);

	public static Color DarkSlateBlue => FromKnownColor(KnownColor.DarkSlateBlue);

	public static Color DarkSlateGray => FromKnownColor(KnownColor.DarkSlateGray);

	public static Color DarkTurquoise => FromKnownColor(KnownColor.DarkTurquoise);

	public static Color DarkViolet => FromKnownColor(KnownColor.DarkViolet);

	public static Color DeepPink => FromKnownColor(KnownColor.DeepPink);

	public static Color DeepSkyBlue => FromKnownColor(KnownColor.DeepSkyBlue);

	public static Color DimGray => FromKnownColor(KnownColor.DimGray);

	public static Color DodgerBlue => FromKnownColor(KnownColor.DodgerBlue);

	public static Color Firebrick => FromKnownColor(KnownColor.Firebrick);

	public static Color FloralWhite => FromKnownColor(KnownColor.FloralWhite);

	public static Color ForestGreen => FromKnownColor(KnownColor.ForestGreen);

	public static Color Fuchsia => FromKnownColor(KnownColor.Fuchsia);

	public static Color Gainsboro => FromKnownColor(KnownColor.Gainsboro);

	public static Color GhostWhite => FromKnownColor(KnownColor.GhostWhite);

	public static Color Gold => FromKnownColor(KnownColor.Gold);

	public static Color Goldenrod => FromKnownColor(KnownColor.Goldenrod);

	public static Color Gray => FromKnownColor(KnownColor.Gray);

	public static Color Green => FromKnownColor(KnownColor.Green);

	public static Color GreenYellow => FromKnownColor(KnownColor.GreenYellow);

	public static Color Honeydew => FromKnownColor(KnownColor.Honeydew);

	public static Color HotPink => FromKnownColor(KnownColor.HotPink);

	public static Color IndianRed => FromKnownColor(KnownColor.IndianRed);

	public static Color Indigo => FromKnownColor(KnownColor.Indigo);

	public static Color Ivory => FromKnownColor(KnownColor.Ivory);

	public static Color Khaki => FromKnownColor(KnownColor.Khaki);

	public static Color Lavender => FromKnownColor(KnownColor.Lavender);

	public static Color LavenderBlush => FromKnownColor(KnownColor.LavenderBlush);

	public static Color LawnGreen => FromKnownColor(KnownColor.LawnGreen);

	public static Color LemonChiffon => FromKnownColor(KnownColor.LemonChiffon);

	public static Color LightBlue => FromKnownColor(KnownColor.LightBlue);

	public static Color LightCoral => FromKnownColor(KnownColor.LightCoral);

	public static Color LightCyan => FromKnownColor(KnownColor.LightCyan);

	public static Color LightGoldenrodYellow => FromKnownColor(KnownColor.LightGoldenrodYellow);

	public static Color LightGreen => FromKnownColor(KnownColor.LightGreen);

	public static Color LightGray => FromKnownColor(KnownColor.LightGray);

	public static Color LightPink => FromKnownColor(KnownColor.LightPink);

	public static Color LightSalmon => FromKnownColor(KnownColor.LightSalmon);

	public static Color LightSeaGreen => FromKnownColor(KnownColor.LightSeaGreen);

	public static Color LightSkyBlue => FromKnownColor(KnownColor.LightSkyBlue);

	public static Color LightSlateGray => FromKnownColor(KnownColor.LightSlateGray);

	public static Color LightSteelBlue => FromKnownColor(KnownColor.LightSteelBlue);

	public static Color LightYellow => FromKnownColor(KnownColor.LightYellow);

	public static Color Lime => FromKnownColor(KnownColor.Lime);

	public static Color LimeGreen => FromKnownColor(KnownColor.LimeGreen);

	public static Color Linen => FromKnownColor(KnownColor.Linen);

	public static Color Magenta => FromKnownColor(KnownColor.Magenta);

	public static Color Maroon => FromKnownColor(KnownColor.Maroon);

	public static Color MediumAquamarine => FromKnownColor(KnownColor.MediumAquamarine);

	public static Color MediumBlue => FromKnownColor(KnownColor.MediumBlue);

	public static Color MediumOrchid => FromKnownColor(KnownColor.MediumOrchid);

	public static Color MediumPurple => FromKnownColor(KnownColor.MediumPurple);

	public static Color MediumSeaGreen => FromKnownColor(KnownColor.MediumSeaGreen);

	public static Color MediumSlateBlue => FromKnownColor(KnownColor.MediumSlateBlue);

	public static Color MediumSpringGreen => FromKnownColor(KnownColor.MediumSpringGreen);

	public static Color MediumTurquoise => FromKnownColor(KnownColor.MediumTurquoise);

	public static Color MediumVioletRed => FromKnownColor(KnownColor.MediumVioletRed);

	public static Color MidnightBlue => FromKnownColor(KnownColor.MidnightBlue);

	public static Color MintCream => FromKnownColor(KnownColor.MintCream);

	public static Color MistyRose => FromKnownColor(KnownColor.MistyRose);

	public static Color Moccasin => FromKnownColor(KnownColor.Moccasin);

	public static Color NavajoWhite => FromKnownColor(KnownColor.NavajoWhite);

	public static Color Navy => FromKnownColor(KnownColor.Navy);

	public static Color OldLace => FromKnownColor(KnownColor.OldLace);

	public static Color Olive => FromKnownColor(KnownColor.Olive);

	public static Color OliveDrab => FromKnownColor(KnownColor.OliveDrab);

	public static Color Orange => FromKnownColor(KnownColor.Orange);

	public static Color OrangeRed => FromKnownColor(KnownColor.OrangeRed);

	public static Color Orchid => FromKnownColor(KnownColor.Orchid);

	public static Color PaleGoldenrod => FromKnownColor(KnownColor.PaleGoldenrod);

	public static Color PaleGreen => FromKnownColor(KnownColor.PaleGreen);

	public static Color PaleTurquoise => FromKnownColor(KnownColor.PaleTurquoise);

	public static Color PaleVioletRed => FromKnownColor(KnownColor.PaleVioletRed);

	public static Color PapayaWhip => FromKnownColor(KnownColor.PapayaWhip);

	public static Color PeachPuff => FromKnownColor(KnownColor.PeachPuff);

	public static Color Peru => FromKnownColor(KnownColor.Peru);

	public static Color Pink => FromKnownColor(KnownColor.Pink);

	public static Color Plum => FromKnownColor(KnownColor.Plum);

	public static Color PowderBlue => FromKnownColor(KnownColor.PowderBlue);

	public static Color Purple => FromKnownColor(KnownColor.Purple);

	public static Color Red => FromKnownColor(KnownColor.Red);

	public static Color RosyBrown => FromKnownColor(KnownColor.RosyBrown);

	public static Color RoyalBlue => FromKnownColor(KnownColor.RoyalBlue);

	public static Color SaddleBrown => FromKnownColor(KnownColor.SaddleBrown);

	public static Color Salmon => FromKnownColor(KnownColor.Salmon);

	public static Color SandyBrown => FromKnownColor(KnownColor.SandyBrown);

	public static Color SeaGreen => FromKnownColor(KnownColor.SeaGreen);

	public static Color SeaShell => FromKnownColor(KnownColor.SeaShell);

	public static Color Sienna => FromKnownColor(KnownColor.Sienna);

	public static Color Silver => FromKnownColor(KnownColor.Silver);

	public static Color SkyBlue => FromKnownColor(KnownColor.SkyBlue);

	public static Color SlateBlue => FromKnownColor(KnownColor.SlateBlue);

	public static Color SlateGray => FromKnownColor(KnownColor.SlateGray);

	public static Color Snow => FromKnownColor(KnownColor.Snow);

	public static Color SpringGreen => FromKnownColor(KnownColor.SpringGreen);

	public static Color SteelBlue => FromKnownColor(KnownColor.SteelBlue);

	public static Color Tan => FromKnownColor(KnownColor.Tan);

	public static Color Teal => FromKnownColor(KnownColor.Teal);

	public static Color Thistle => FromKnownColor(KnownColor.Thistle);

	public static Color Tomato => FromKnownColor(KnownColor.Tomato);

	public static Color Turquoise => FromKnownColor(KnownColor.Turquoise);

	public static Color Violet => FromKnownColor(KnownColor.Violet);

	public static Color Wheat => FromKnownColor(KnownColor.Wheat);

	public static Color White => FromKnownColor(KnownColor.White);

	public static Color WhiteSmoke => FromKnownColor(KnownColor.WhiteSmoke);

	public static Color Yellow => FromKnownColor(KnownColor.Yellow);

	public static Color YellowGreen => FromKnownColor(KnownColor.YellowGreen);

	public static Color FromArgb(int red, int green, int blue)
	{
		return FromArgb(255, red, green, blue);
	}

	public static Color FromArgb(int alpha, int red, int green, int blue)
	{
		CheckARGBValues(alpha, red, green, blue);
		Color result = default(Color);
		result.state = 2;
		result.Value = (alpha << 24) + (red << 16) + (green << 8) + blue;
		return result;
	}

	public int ToArgb()
	{
		return (int)Value;
	}

	public static Color FromArgb(int alpha, Color baseColor)
	{
		return FromArgb(alpha, baseColor.R, baseColor.G, baseColor.B);
	}

	public static Color FromArgb(int argb)
	{
		return FromArgb((argb >> 24) & 0xFF, (argb >> 16) & 0xFF, (argb >> 8) & 0xFF, argb & 0xFF);
	}

	public static Color FromKnownColor(KnownColor color)
	{
		short num = (short)color;
		Color result;
		if (num <= 0 || num >= KnownColors.ArgbValues.Length)
		{
			result = FromArgb(0, 0, 0, 0);
			result.state |= 4;
		}
		else
		{
			result = default(Color);
			result.state = 7;
			if (num < 27 || num > 169)
			{
				result.state |= 8;
			}
			result.Value = KnownColors.ArgbValues[num];
		}
		result.knownColor = num;
		return result;
	}

	public static Color FromName(string name)
	{
		try
		{
			return FromKnownColor((KnownColor)Enum.Parse(typeof(KnownColor), name, ignoreCase: true));
		}
		catch
		{
			Color result = FromArgb(0, 0, 0, 0);
			result.name = name;
			result.state |= 4;
			return result;
		}
	}

	public static bool operator ==(Color left, Color right)
	{
		if (left.Value != right.Value)
		{
			return false;
		}
		if (left.IsNamedColor != right.IsNamedColor)
		{
			return false;
		}
		if (left.IsSystemColor != right.IsSystemColor)
		{
			return false;
		}
		if (left.IsEmpty != right.IsEmpty)
		{
			return false;
		}
		if (left.IsNamedColor && left.Name != right.Name)
		{
			return false;
		}
		return true;
	}

	public static bool operator !=(Color left, Color right)
	{
		return !(left == right);
	}

	public float GetBrightness()
	{
		byte b = Math.Min(R, Math.Min(G, B));
		return (float)(Math.Max(R, Math.Max(G, B)) + b) / 510f;
	}

	public float GetSaturation()
	{
		byte b = Math.Min(R, Math.Min(G, B));
		byte b2 = Math.Max(R, Math.Max(G, B));
		if (b2 == b)
		{
			return 0f;
		}
		int num = b2 + b;
		if (num > 255)
		{
			num = 510 - num;
		}
		return (float)(b2 - b) / (float)num;
	}

	public float GetHue()
	{
		int r = R;
		int g = G;
		int b = B;
		byte b2 = (byte)Math.Min(r, Math.Min(g, b));
		byte b3 = (byte)Math.Max(r, Math.Max(g, b));
		if (b3 == b2)
		{
			return 0f;
		}
		float num = b3 - b2;
		float num2 = (float)(b3 - r) / num;
		float num3 = (float)(b3 - g) / num;
		float num4 = (float)(b3 - b) / num;
		float num5 = 0f;
		if (r == b3)
		{
			num5 = 60f * (6f + num4 - num3);
		}
		if (g == b3)
		{
			num5 = 60f * (2f + num2 - num4);
		}
		if (b == b3)
		{
			num5 = 60f * (4f + num3 - num2);
		}
		if (num5 > 360f)
		{
			num5 -= 360f;
		}
		return num5;
	}

	public KnownColor ToKnownColor()
	{
		return (KnownColor)knownColor;
	}

	public override bool Equals(object obj)
	{
		if (!(obj is Color color))
		{
			return false;
		}
		return this == color;
	}

	public override int GetHashCode()
	{
		int num = (int)(Value ^ (Value >> 32) ^ state ^ (knownColor >> 16));
		if (IsNamedColor)
		{
			num ^= Name.GetHashCode();
		}
		return num;
	}

	public override string ToString()
	{
		if (IsEmpty)
		{
			return "Color [Empty]";
		}
		if (IsNamedColor)
		{
			return "Color [" + Name + "]";
		}
		return $"Color [A={A}, R={R}, G={G}, B={B}]";
	}

	private static void CheckRGBValues(int red, int green, int blue)
	{
		if (red > 255 || red < 0)
		{
			throw CreateColorArgumentException(red, "red");
		}
		if (green > 255 || green < 0)
		{
			throw CreateColorArgumentException(green, "green");
		}
		if (blue > 255 || blue < 0)
		{
			throw CreateColorArgumentException(blue, "blue");
		}
	}

	private static ArgumentException CreateColorArgumentException(int value, string color)
	{
		return new ArgumentException(string.Format("'{0}' is not a valid value for '{1}'. '{1}' should be greater or equal to 0 and less than or equal to 255.", value, color));
	}

	private static void CheckARGBValues(int alpha, int red, int green, int blue)
	{
		if (alpha > 255 || alpha < 0)
		{
			throw CreateColorArgumentException(alpha, "alpha");
		}
		CheckRGBValues(red, green, blue);
	}
}
