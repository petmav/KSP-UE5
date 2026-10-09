using System;
using System.Reflection;
using System.Runtime.CompilerServices;

namespace I18N.CJK;

internal class GB18030Source
{
	private class GB18030Map
	{
		public readonly int UStart;

		public readonly int UEnd;

		public readonly long GStart;

		public readonly long GEnd;

		public readonly bool Dummy;

		public GB18030Map(int ustart, int uend, long gstart, long gend, bool dummy)
		{
			UStart = ustart;
			UEnd = uend;
			GStart = gstart;
			GEnd = gend;
			Dummy = dummy;
		}
	}

	private unsafe static readonly byte* gbx2uni;

	private unsafe static readonly byte* uni2gbx;

	private static readonly int gbx2uniSize;

	private static readonly int uni2gbxSize;

	private static readonly long gbxBase;

	private static readonly long gbxSuppBase;

	private static readonly GB18030Map[] ranges;

	private GB18030Source()
	{
	}

	unsafe static GB18030Source()
	{
		gbxBase = FromGBXRaw(129, 48, 129, 48, supp: false);
		gbxSuppBase = FromGBXRaw(144, 48, 129, 48, supp: false);
		ranges = new GB18030Map[14]
		{
			new GB18030Map(1106, 8207, FromGBXRaw(129, 48, 211, 48, supp: false), FromGBXRaw(129, 54, 165, 49, supp: false), dummy: false),
			new GB18030Map(9795, 11904, FromGBXRaw(129, 55, 168, 57, supp: false), FromGBXRaw(129, 56, 253, 56, supp: false), dummy: false),
			new GB18030Map(13851, 14615, FromGBXRaw(130, 48, 166, 51, supp: false), FromGBXRaw(130, 48, 242, 55, supp: false), dummy: false),
			new GB18030Map(15585, 16469, FromGBXRaw(130, 49, 212, 56, supp: false), FromGBXRaw(130, 50, 175, 50, supp: false), dummy: false),
			new GB18030Map(16736, 17206, FromGBXRaw(130, 50, 201, 55, supp: false), FromGBXRaw(130, 50, 248, 55, supp: false), dummy: false),
			new GB18030Map(17623, 17995, FromGBXRaw(130, 51, 163, 57, supp: false), FromGBXRaw(130, 51, 201, 49, supp: false), dummy: false),
			new GB18030Map(18318, 18758, FromGBXRaw(130, 51, 232, 56, supp: false), FromGBXRaw(130, 52, 150, 56, supp: false), dummy: false),
			new GB18030Map(18872, 19574, FromGBXRaw(130, 52, 161, 49, supp: false), FromGBXRaw(130, 52, 231, 51, supp: false), dummy: false),
			new GB18030Map(19968, 40869, 0L, 0L, dummy: true),
			new GB18030Map(40870, 55295, FromGBXRaw(130, 53, 143, 51, supp: false), FromGBXRaw(131, 54, 199, 56, supp: false), dummy: false),
			new GB18030Map(55296, 59243, 0L, 0L, dummy: true),
			new GB18030Map(59493, 63787, FromGBXRaw(131, 54, 208, 48, supp: false), FromGBXRaw(132, 48, 133, 52, supp: false), dummy: false),
			new GB18030Map(64042, 65071, FromGBXRaw(132, 48, 156, 56, supp: false), FromGBXRaw(132, 49, 133, 55, supp: false), dummy: false),
			new GB18030Map(65510, 65535, FromGBXRaw(132, 49, 162, 52, supp: false), FromGBXRaw(132, 49, 164, 57, supp: false), dummy: false)
		};
		MethodInfo method = typeof(Assembly).GetMethod("GetManifestResourceInternal", BindingFlags.Instance | BindingFlags.NonPublic);
		int num = 0;
		Module module = null;
		IntPtr intPtr = (IntPtr)method.Invoke(Assembly.GetExecutingAssembly(), new object[3] { "gb18030.table", num, module });
		if (intPtr != IntPtr.Zero)
		{
			gbx2uni = (byte*)(void*)intPtr;
			gbx2uniSize = (*gbx2uni << 24) + (gbx2uni[1] << 16) + (gbx2uni[2] << 8) + gbx2uni[3];
			gbx2uni += 4;
			uni2gbx = gbx2uni + gbx2uniSize;
			uni2gbxSize = (*uni2gbx << 24) + (uni2gbx[1] << 16) + (uni2gbx[2] << 8) + uni2gbx[3];
			uni2gbx += 4;
		}
	}

	public unsafe static void Unlinear(byte[] bytes, int start, long gbx)
	{
		//IL_0015->IL001c: Incompatible stack types: I vs Ref
		fixed (byte* ptr = &System.Runtime.CompilerServices.Unsafe.AsRef<byte>((byte*)System.Runtime.CompilerServices.Unsafe.AsPointer(ref bytes != null && bytes.Length != 0 ? ref System.Runtime.CompilerServices.Unsafe.As<byte, _003F>(ref bytes[0]) : ref *(_003F*)null)))
		{
			Unlinear(ptr + start, gbx);
		}
	}

	public unsafe static void Unlinear(byte* bytes, long gbx)
	{
		bytes[3] = (byte)(gbx % 10 + 48);
		gbx /= 10;
		bytes[2] = (byte)(gbx % 126 + 129);
		gbx /= 126;
		bytes[1] = (byte)(gbx % 10 + 48);
		gbx /= 10;
		*bytes = (byte)(gbx + 129);
	}

	public static long FromGBX(byte[] bytes, int start)
	{
		byte b = bytes[start];
		byte b2 = bytes[start + 1];
		byte b3 = bytes[start + 2];
		byte b4 = bytes[start + 3];
		if (b < 129 || b == byte.MaxValue)
		{
			return -1L;
		}
		if (b2 < 48 || b2 > 57)
		{
			return -2L;
		}
		if (b3 < 129 || b3 == byte.MaxValue)
		{
			return -3L;
		}
		if (b4 < 48 || b4 > 57)
		{
			return -4L;
		}
		if (b >= 144)
		{
			return FromGBXRaw(b, b2, b3, b4, supp: true);
		}
		long num = FromGBXRaw(b, b2, b3, b4, supp: false);
		long num2 = 0L;
		long num3 = 0L;
		for (int i = 0; i < ranges.Length; i++)
		{
			GB18030Map gB18030Map = ranges[i];
			if (num < gB18030Map.GStart)
			{
				return ToUcsRaw((int)(num - num3 + num2));
			}
			if (num <= gB18030Map.GEnd)
			{
				return num - gbxBase - gB18030Map.GStart + gB18030Map.UStart;
			}
			if (gB18030Map.GStart != 0L)
			{
				num2 += gB18030Map.GStart - num3;
				num3 = gB18030Map.GEnd + 1;
			}
		}
		throw new SystemException($"GB18030 INTERNAL ERROR (should not happen): GBX {b:x02} {b2:x02} {b3:x02} {b4:x02}");
	}

	public static long FromUCSSurrogate(int cp)
	{
		return cp + gbxSuppBase;
	}

	public static long FromUCS(int cp)
	{
		long num = 0L;
		long num2 = 128L;
		for (int i = 0; i < ranges.Length; i++)
		{
			GB18030Map gB18030Map = ranges[i];
			if (cp < gB18030Map.UStart)
			{
				return ToGbxRaw((int)(cp - num2 + num));
			}
			if (cp <= gB18030Map.UEnd)
			{
				return cp - gB18030Map.UStart + gB18030Map.GStart;
			}
			if (gB18030Map.GStart != 0L)
			{
				num += gB18030Map.UStart - num2;
				num2 = gB18030Map.UEnd + 1;
			}
		}
		throw new SystemException($"GB18030 INTERNAL ERROR (should not happen): UCS {cp:x06}");
	}

	private static long FromGBXRaw(byte b1, byte b2, byte b3, byte b4, bool supp)
	{
		return (((b1 - ((!supp) ? 129 : 144)) * 10 + (b2 - 48)) * 126 + (b3 - 129)) * 10 + b4 - 48 + (supp ? 65536 : 0);
	}

	private unsafe static int ToUcsRaw(int idx)
	{
		return gbx2uni[idx * 2] * 256 + gbx2uni[idx * 2 + 1];
	}

	private unsafe static long ToGbxRaw(int idx)
	{
		if (idx < 0 || idx * 2 + 1 >= uni2gbxSize)
		{
			return -1L;
		}
		return gbxBase + uni2gbx[idx * 2] * 256 + (int)uni2gbx[idx * 2 + 1];
	}
}
