using System.Reflection;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace System;

internal static class SpanHelpers
{
	[StructLayout(LayoutKind.Sequential, Size = 64)]
	private struct Reg64
	{
	}

	[StructLayout(LayoutKind.Sequential, Size = 32)]
	private struct Reg32
	{
	}

	[StructLayout(LayoutKind.Sequential, Size = 16)]
	private struct Reg16
	{
	}

	public static class PerTypeValues<T>
	{
		public static readonly bool IsReferenceOrContainsReferences = IsReferenceOrContainsReferencesCore(typeof(T));

		public static readonly T[] EmptyArray = new T[0];

		public static readonly IntPtr ArrayAdjustment = MeasureArrayAdjustment();

		private static IntPtr MeasureArrayAdjustment()
		{
			T[] array = new T[1];
			return Unsafe.ByteOffset(ref Unsafe.As<Pinnable<T>>(array).Data, ref array[0]);
		}
	}

	public unsafe static void ClearLessThanPointerSized(byte* ptr, UIntPtr byteLength)
	{
		if (sizeof(UIntPtr) == 4)
		{
			Unsafe.InitBlockUnaligned(ptr, 0, (uint)byteLength);
			return;
		}
		ulong num = (ulong)byteLength;
		uint num2 = (uint)(num & 0xFFFFFFFFu);
		Unsafe.InitBlockUnaligned(ptr, 0, num2);
		num -= num2;
		ptr += num2;
		while (num != 0)
		{
			num2 = (uint)((num >= uint.MaxValue) ? uint.MaxValue : num);
			Unsafe.InitBlockUnaligned(ptr, 0, num2);
			ptr += num2;
			num -= num2;
		}
	}

	public unsafe static void ClearLessThanPointerSized(ref byte b, UIntPtr byteLength)
	{
		if (sizeof(UIntPtr) == 4)
		{
			Unsafe.InitBlockUnaligned(ref b, 0, (uint)byteLength);
			return;
		}
		ulong num = (ulong)byteLength;
		uint num2 = (uint)(num & 0xFFFFFFFFu);
		Unsafe.InitBlockUnaligned(ref b, 0, num2);
		num -= num2;
		long num3 = num2;
		while (num != 0)
		{
			num2 = (uint)((num >= uint.MaxValue) ? uint.MaxValue : num);
			Unsafe.InitBlockUnaligned(ref Unsafe.Add(ref b, (IntPtr)num3), 0, num2);
			num3 += num2;
			num -= num2;
		}
	}

	public unsafe static void ClearPointerSizedWithoutReferences(ref byte b, UIntPtr byteLength)
	{
		IntPtr zero;
		for (zero = IntPtr.Zero; zero.LessThanEqual(byteLength - sizeof(Reg64)); zero += sizeof(Reg64))
		{
			Unsafe.As<byte, Reg64>(ref Unsafe.Add(ref b, zero)) = default(Reg64);
		}
		if (zero.LessThanEqual(byteLength - sizeof(Reg32)))
		{
			Unsafe.As<byte, Reg32>(ref Unsafe.Add(ref b, zero)) = default(Reg32);
			zero += sizeof(Reg32);
		}
		if (zero.LessThanEqual(byteLength - sizeof(Reg16)))
		{
			Unsafe.As<byte, Reg16>(ref Unsafe.Add(ref b, zero)) = default(Reg16);
			zero += sizeof(Reg16);
		}
		if (zero.LessThanEqual(byteLength - 8))
		{
			Unsafe.As<byte, long>(ref Unsafe.Add(ref b, zero)) = 0L;
			zero += 8;
		}
		if (sizeof(IntPtr) == 4 && zero.LessThanEqual(byteLength - 4))
		{
			Unsafe.As<byte, int>(ref Unsafe.Add(ref b, zero)) = 0;
			zero += 4;
		}
	}

	public static void ClearPointerSizedWithReferences(ref IntPtr ip, UIntPtr pointerSizeLength)
	{
		IntPtr intPtr = IntPtr.Zero;
		IntPtr zero = IntPtr.Zero;
		while ((zero = intPtr + 8).LessThanEqual(pointerSizeLength))
		{
			Unsafe.Add(ref ip, intPtr + 0) = default(IntPtr);
			Unsafe.Add(ref ip, intPtr + 1) = default(IntPtr);
			Unsafe.Add(ref ip, intPtr + 2) = default(IntPtr);
			Unsafe.Add(ref ip, intPtr + 3) = default(IntPtr);
			Unsafe.Add(ref ip, intPtr + 4) = default(IntPtr);
			Unsafe.Add(ref ip, intPtr + 5) = default(IntPtr);
			Unsafe.Add(ref ip, intPtr + 6) = default(IntPtr);
			Unsafe.Add(ref ip, intPtr + 7) = default(IntPtr);
			intPtr = zero;
		}
		if ((zero = intPtr + 4).LessThanEqual(pointerSizeLength))
		{
			Unsafe.Add(ref ip, intPtr + 0) = default(IntPtr);
			Unsafe.Add(ref ip, intPtr + 1) = default(IntPtr);
			Unsafe.Add(ref ip, intPtr + 2) = default(IntPtr);
			Unsafe.Add(ref ip, intPtr + 3) = default(IntPtr);
			intPtr = zero;
		}
		if ((zero = intPtr + 2).LessThanEqual(pointerSizeLength))
		{
			Unsafe.Add(ref ip, intPtr + 0) = default(IntPtr);
			Unsafe.Add(ref ip, intPtr + 1) = default(IntPtr);
			intPtr = zero;
		}
		if ((intPtr + 1).LessThanEqual(pointerSizeLength))
		{
			Unsafe.Add(ref ip, intPtr) = default(IntPtr);
		}
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	private unsafe static bool LessThanEqual(this IntPtr index, UIntPtr length)
	{
		if (sizeof(UIntPtr) != 4)
		{
			return (long)index <= (long)(ulong)length;
		}
		return (int)index <= (int)(uint)length;
	}

	public static int IndexOf<T>(ref T searchSpace, int searchSpaceLength, ref T value, int valueLength) where T : struct, IEquatable<T>
	{
		if (valueLength == 0)
		{
			return 0;
		}
		T value2 = value;
		ref T second = ref Unsafe.Add(ref value, 1);
		int num = valueLength - 1;
		int num2 = 0;
		while (true)
		{
			int num3 = searchSpaceLength - num2 - num;
			if (num3 <= 0)
			{
				break;
			}
			int num4 = IndexOf(ref Unsafe.Add(ref searchSpace, num2), value2, num3);
			if (num4 == -1)
			{
				break;
			}
			num2 += num4;
			if (SequenceEqual(ref Unsafe.Add(ref searchSpace, num2 + 1), ref second, num))
			{
				return num2;
			}
			num2++;
		}
		return -1;
	}

	public unsafe static int IndexOf<T>(ref T searchSpace, T value, int length) where T : struct, IEquatable<T>
	{
		IntPtr intPtr = (IntPtr)0;
		while (true)
		{
			if (length >= 8)
			{
				length -= 8;
				if (!value.Equals(Unsafe.Add(ref searchSpace, intPtr)))
				{
					if (value.Equals(Unsafe.Add(ref searchSpace, intPtr + 1)))
					{
						goto IL_020a;
					}
					if (value.Equals(Unsafe.Add(ref searchSpace, intPtr + 2)))
					{
						goto IL_0218;
					}
					if (!value.Equals(Unsafe.Add(ref searchSpace, intPtr + 3)))
					{
						if (!value.Equals(Unsafe.Add(ref searchSpace, intPtr + 4)))
						{
							if (!value.Equals(Unsafe.Add(ref searchSpace, intPtr + 5)))
							{
								if (!value.Equals(Unsafe.Add(ref searchSpace, intPtr + 6)))
								{
									if (value.Equals(Unsafe.Add(ref searchSpace, intPtr + 7)))
									{
										break;
									}
									intPtr += 8;
									continue;
								}
								return (int)(void*)(intPtr + 6);
							}
							return (int)(void*)(intPtr + 5);
						}
						return (int)(void*)(intPtr + 4);
					}
					goto IL_0226;
				}
			}
			else
			{
				if (length >= 4)
				{
					length -= 4;
					if (value.Equals(Unsafe.Add(ref searchSpace, intPtr)))
					{
						goto IL_0202;
					}
					if (value.Equals(Unsafe.Add(ref searchSpace, intPtr + 1)))
					{
						goto IL_020a;
					}
					if (value.Equals(Unsafe.Add(ref searchSpace, intPtr + 2)))
					{
						goto IL_0218;
					}
					if (value.Equals(Unsafe.Add(ref searchSpace, intPtr + 3)))
					{
						goto IL_0226;
					}
					intPtr += 4;
				}
				while (true)
				{
					if (length > 0)
					{
						if (value.Equals(Unsafe.Add(ref searchSpace, intPtr)))
						{
							break;
						}
						intPtr += 1;
						length--;
						continue;
					}
					return -1;
				}
			}
			goto IL_0202;
			IL_0218:
			return (int)(void*)(intPtr + 2);
			IL_0202:
			return (int)(void*)intPtr;
			IL_020a:
			return (int)(void*)(intPtr + 1);
			IL_0226:
			return (int)(void*)(intPtr + 3);
		}
		return (int)(void*)(intPtr + 7);
	}

	public static bool SequenceEqual<T>(ref T first, ref T second, int length) where T : struct, IEquatable<T>
	{
		if (!Unsafe.AreSame(ref first, ref second))
		{
			IntPtr intPtr = (IntPtr)0;
			while (true)
			{
				if (length >= 8)
				{
					length -= 8;
					if (Unsafe.Add(ref first, intPtr).Equals(Unsafe.Add(ref second, intPtr)) && Unsafe.Add(ref first, intPtr + 1).Equals(Unsafe.Add(ref second, intPtr + 1)) && Unsafe.Add(ref first, intPtr + 2).Equals(Unsafe.Add(ref second, intPtr + 2)) && Unsafe.Add(ref first, intPtr + 3).Equals(Unsafe.Add(ref second, intPtr + 3)) && Unsafe.Add(ref first, intPtr + 4).Equals(Unsafe.Add(ref second, intPtr + 4)) && Unsafe.Add(ref first, intPtr + 5).Equals(Unsafe.Add(ref second, intPtr + 5)) && Unsafe.Add(ref first, intPtr + 6).Equals(Unsafe.Add(ref second, intPtr + 6)) && Unsafe.Add(ref first, intPtr + 7).Equals(Unsafe.Add(ref second, intPtr + 7)))
					{
						intPtr += 8;
						continue;
					}
					goto IL_028b;
				}
				if (length >= 4)
				{
					length -= 4;
					if (!Unsafe.Add(ref first, intPtr).Equals(Unsafe.Add(ref second, intPtr)) || !Unsafe.Add(ref first, intPtr + 1).Equals(Unsafe.Add(ref second, intPtr + 1)) || !Unsafe.Add(ref first, intPtr + 2).Equals(Unsafe.Add(ref second, intPtr + 2)) || !Unsafe.Add(ref first, intPtr + 3).Equals(Unsafe.Add(ref second, intPtr + 3)))
					{
						goto IL_028b;
					}
					intPtr += 4;
				}
				while (length > 0)
				{
					if (Unsafe.Add(ref first, intPtr).Equals(Unsafe.Add(ref second, intPtr)))
					{
						intPtr += 1;
						length--;
						continue;
					}
					goto IL_028b;
				}
				break;
				IL_028b:
				return false;
			}
		}
		return true;
	}

	public static int IndexOf(ref byte searchSpace, int searchSpaceLength, ref byte value, int valueLength)
	{
		if (valueLength == 0)
		{
			return 0;
		}
		byte value2 = value;
		ref byte second = ref Unsafe.Add(ref value, 1);
		int num = valueLength - 1;
		int num2 = 0;
		while (true)
		{
			int num3 = searchSpaceLength - num2 - num;
			if (num3 <= 0)
			{
				break;
			}
			int num4 = IndexOf(ref Unsafe.Add(ref searchSpace, num2), value2, num3);
			if (num4 == -1)
			{
				break;
			}
			num2 += num4;
			if (SequenceEqual(ref Unsafe.Add(ref searchSpace, num2 + 1), ref second, num))
			{
				return num2;
			}
			num2++;
		}
		return -1;
	}

	public static int IndexOfAny(ref byte searchSpace, int searchSpaceLength, ref byte value, int valueLength)
	{
		if (valueLength == 0)
		{
			return 0;
		}
		int num = -1;
		for (int i = 0; i < valueLength; i++)
		{
			int num2 = IndexOf(ref searchSpace, Unsafe.Add(ref value, i), searchSpaceLength);
			if (num2 != -1)
			{
				num = ((num == -1 || num > num2) ? num2 : num);
			}
		}
		return num;
	}

	public unsafe static int IndexOf(ref byte searchSpace, byte value, int length)
	{
		IntPtr intPtr = (IntPtr)0;
		IntPtr intPtr2 = (IntPtr)(uint)length;
		while (true)
		{
			if ((nuint)(void*)intPtr2 >= (nuint)8u)
			{
				intPtr2 -= 8;
				if (value != Unsafe.Add(ref searchSpace, intPtr))
				{
					if (value == Unsafe.Add(ref searchSpace, intPtr + 1))
					{
						goto IL_0156;
					}
					if (value == Unsafe.Add(ref searchSpace, intPtr + 2))
					{
						goto IL_0164;
					}
					if (value != Unsafe.Add(ref searchSpace, intPtr + 3))
					{
						if (value != Unsafe.Add(ref searchSpace, intPtr + 4))
						{
							if (value != Unsafe.Add(ref searchSpace, intPtr + 5))
							{
								if (value != Unsafe.Add(ref searchSpace, intPtr + 6))
								{
									if (value == Unsafe.Add(ref searchSpace, intPtr + 7))
									{
										break;
									}
									intPtr += 8;
									continue;
								}
								return (int)(void*)(intPtr + 6);
							}
							return (int)(void*)(intPtr + 5);
						}
						return (int)(void*)(intPtr + 4);
					}
					goto IL_0172;
				}
			}
			else
			{
				if ((nuint)(void*)intPtr2 >= (nuint)4u)
				{
					intPtr2 -= 4;
					if (value == Unsafe.Add(ref searchSpace, intPtr))
					{
						goto IL_014e;
					}
					if (value == Unsafe.Add(ref searchSpace, intPtr + 1))
					{
						goto IL_0156;
					}
					if (value == Unsafe.Add(ref searchSpace, intPtr + 2))
					{
						goto IL_0164;
					}
					if (value == Unsafe.Add(ref searchSpace, intPtr + 3))
					{
						goto IL_0172;
					}
					intPtr += 4;
				}
				while (true)
				{
					if ((void*)intPtr2 != null)
					{
						intPtr2 -= 1;
						if (value == Unsafe.Add(ref searchSpace, intPtr))
						{
							break;
						}
						intPtr += 1;
						continue;
					}
					return -1;
				}
			}
			goto IL_014e;
			IL_0164:
			return (int)(void*)(intPtr + 2);
			IL_014e:
			return (int)(void*)intPtr;
			IL_0156:
			return (int)(void*)(intPtr + 1);
			IL_0172:
			return (int)(void*)(intPtr + 3);
		}
		return (int)(void*)(intPtr + 7);
	}

	public unsafe static int IndexOfAny(ref byte searchSpace, byte value0, byte value1, int length)
	{
		IntPtr intPtr = (IntPtr)0;
		IntPtr intPtr2 = (IntPtr)(uint)length;
		while (true)
		{
			if ((nuint)(void*)intPtr2 >= (nuint)8u)
			{
				intPtr2 -= 8;
				uint num = Unsafe.Add(ref searchSpace, intPtr);
				if (value0 != num && value1 != num)
				{
					num = Unsafe.Add(ref searchSpace, intPtr + 1);
					if (value0 == num || value1 == num)
					{
						goto IL_01ee;
					}
					num = Unsafe.Add(ref searchSpace, intPtr + 2);
					if (value0 == num || value1 == num)
					{
						goto IL_01fc;
					}
					num = Unsafe.Add(ref searchSpace, intPtr + 3);
					if (value0 != num && value1 != num)
					{
						num = Unsafe.Add(ref searchSpace, intPtr + 4);
						if (value0 != num && value1 != num)
						{
							num = Unsafe.Add(ref searchSpace, intPtr + 5);
							if (value0 != num && value1 != num)
							{
								num = Unsafe.Add(ref searchSpace, intPtr + 6);
								if (value0 != num && value1 != num)
								{
									num = Unsafe.Add(ref searchSpace, intPtr + 7);
									if (value0 == num || value1 == num)
									{
										break;
									}
									intPtr += 8;
									continue;
								}
								return (int)(void*)(intPtr + 6);
							}
							return (int)(void*)(intPtr + 5);
						}
						return (int)(void*)(intPtr + 4);
					}
					goto IL_020a;
				}
			}
			else
			{
				if ((nuint)(void*)intPtr2 >= (nuint)4u)
				{
					intPtr2 -= 4;
					uint num = Unsafe.Add(ref searchSpace, intPtr);
					if (value0 == num || value1 == num)
					{
						goto IL_01e6;
					}
					num = Unsafe.Add(ref searchSpace, intPtr + 1);
					if (value0 == num || value1 == num)
					{
						goto IL_01ee;
					}
					num = Unsafe.Add(ref searchSpace, intPtr + 2);
					if (value0 == num || value1 == num)
					{
						goto IL_01fc;
					}
					num = Unsafe.Add(ref searchSpace, intPtr + 3);
					if (value0 == num || value1 == num)
					{
						goto IL_020a;
					}
					intPtr += 4;
				}
				while (true)
				{
					if ((void*)intPtr2 != null)
					{
						intPtr2 -= 1;
						uint num = Unsafe.Add(ref searchSpace, intPtr);
						if (value0 == num || value1 == num)
						{
							break;
						}
						intPtr += 1;
						continue;
					}
					return -1;
				}
			}
			goto IL_01e6;
			IL_01fc:
			return (int)(void*)(intPtr + 2);
			IL_01ee:
			return (int)(void*)(intPtr + 1);
			IL_020a:
			return (int)(void*)(intPtr + 3);
			IL_01e6:
			return (int)(void*)intPtr;
		}
		return (int)(void*)(intPtr + 7);
	}

	public unsafe static int IndexOfAny(ref byte searchSpace, byte value0, byte value1, byte value2, int length)
	{
		IntPtr intPtr = (IntPtr)0;
		IntPtr intPtr2 = (IntPtr)(uint)length;
		while (true)
		{
			if ((nuint)(void*)intPtr2 >= (nuint)8u)
			{
				intPtr2 -= 8;
				uint num = Unsafe.Add(ref searchSpace, intPtr);
				if (value0 != num && value1 != num && value2 != num)
				{
					num = Unsafe.Add(ref searchSpace, intPtr + 1);
					if (value0 == num || value1 == num || value2 == num)
					{
						goto IL_0263;
					}
					num = Unsafe.Add(ref searchSpace, intPtr + 2);
					if (value0 == num || value1 == num || value2 == num)
					{
						goto IL_0271;
					}
					num = Unsafe.Add(ref searchSpace, intPtr + 3);
					if (value0 != num && value1 != num && value2 != num)
					{
						num = Unsafe.Add(ref searchSpace, intPtr + 4);
						if (value0 != num && value1 != num && value2 != num)
						{
							num = Unsafe.Add(ref searchSpace, intPtr + 5);
							if (value0 != num && value1 != num && value2 != num)
							{
								num = Unsafe.Add(ref searchSpace, intPtr + 6);
								if (value0 != num && value1 != num && value2 != num)
								{
									num = Unsafe.Add(ref searchSpace, intPtr + 7);
									if (value0 == num || value1 == num || value2 == num)
									{
										break;
									}
									intPtr += 8;
									continue;
								}
								return (int)(void*)(intPtr + 6);
							}
							return (int)(void*)(intPtr + 5);
						}
						return (int)(void*)(intPtr + 4);
					}
					goto IL_027f;
				}
			}
			else
			{
				if ((nuint)(void*)intPtr2 >= (nuint)4u)
				{
					intPtr2 -= 4;
					uint num = Unsafe.Add(ref searchSpace, intPtr);
					if (value0 == num || value1 == num || value2 == num)
					{
						goto IL_025b;
					}
					num = Unsafe.Add(ref searchSpace, intPtr + 1);
					if (value0 == num || value1 == num || value2 == num)
					{
						goto IL_0263;
					}
					num = Unsafe.Add(ref searchSpace, intPtr + 2);
					if (value0 == num || value1 == num || value2 == num)
					{
						goto IL_0271;
					}
					num = Unsafe.Add(ref searchSpace, intPtr + 3);
					if (value0 == num || value1 == num || value2 == num)
					{
						goto IL_027f;
					}
					intPtr += 4;
				}
				while (true)
				{
					if ((void*)intPtr2 != null)
					{
						intPtr2 -= 1;
						uint num = Unsafe.Add(ref searchSpace, intPtr);
						if (value0 == num || value1 == num || value2 == num)
						{
							break;
						}
						intPtr += 1;
						continue;
					}
					return -1;
				}
			}
			goto IL_025b;
			IL_025b:
			return (int)(void*)intPtr;
			IL_0271:
			return (int)(void*)(intPtr + 2);
			IL_0263:
			return (int)(void*)(intPtr + 1);
			IL_027f:
			return (int)(void*)(intPtr + 3);
		}
		return (int)(void*)(intPtr + 7);
	}

	public unsafe static bool SequenceEqual(ref byte first, ref byte second, int length)
	{
		if (!Unsafe.AreSame(ref first, ref second))
		{
			IntPtr intPtr = (IntPtr)0;
			IntPtr intPtr2 = (IntPtr)length;
			if ((nuint)(void*)intPtr2 >= (nuint)sizeof(UIntPtr))
			{
				intPtr2 -= sizeof(UIntPtr);
				while (true)
				{
					if ((void*)intPtr2 > (void*)intPtr)
					{
						if (Unsafe.ReadUnaligned<UIntPtr>(ref Unsafe.AddByteOffset(ref first, intPtr)) != Unsafe.ReadUnaligned<UIntPtr>(ref Unsafe.AddByteOffset(ref second, intPtr)))
						{
							break;
						}
						intPtr += sizeof(UIntPtr);
						continue;
					}
					return Unsafe.ReadUnaligned<UIntPtr>(ref Unsafe.AddByteOffset(ref first, intPtr2)) == Unsafe.ReadUnaligned<UIntPtr>(ref Unsafe.AddByteOffset(ref second, intPtr2));
				}
				goto IL_00bd;
			}
			while ((void*)intPtr2 > (void*)intPtr)
			{
				if (Unsafe.AddByteOffset(ref first, intPtr) == Unsafe.AddByteOffset(ref second, intPtr))
				{
					intPtr += 1;
					continue;
				}
				goto IL_00bd;
			}
		}
		return true;
		IL_00bd:
		return false;
	}

	public unsafe static void CopyTo<T>(ref T dst, int dstLength, ref T src, int srcLength)
	{
		IntPtr intPtr = Unsafe.ByteOffset(ref src, ref Unsafe.Add(ref src, srcLength));
		IntPtr intPtr2 = Unsafe.ByteOffset(ref dst, ref Unsafe.Add(ref dst, dstLength));
		IntPtr intPtr3 = Unsafe.ByteOffset(ref src, ref dst);
		bool num;
		if (sizeof(IntPtr) != 4)
		{
			if ((ulong)(long)intPtr3 >= (ulong)(long)intPtr)
			{
				num = (ulong)(long)intPtr3 > (ulong)(-(long)intPtr2);
				goto IL_006f;
			}
		}
		else if ((uint)(int)intPtr3 >= (uint)(int)intPtr)
		{
			num = (uint)(int)intPtr3 > (uint)(-(int)intPtr2);
			goto IL_006f;
		}
		goto IL_00da;
		IL_006f:
		if (!num && !IsReferenceOrContainsReferences<T>())
		{
			ref byte source = ref Unsafe.As<T, byte>(ref dst);
			ref byte source2 = ref Unsafe.As<T, byte>(ref src);
			ulong num2 = (ulong)(long)intPtr;
			uint num4;
			for (ulong num3 = 0uL; num3 < num2; num3 += num4)
			{
				num4 = (uint)((num2 - num3 > uint.MaxValue) ? uint.MaxValue : (num2 - num3));
				Unsafe.CopyBlock(ref Unsafe.Add(ref source, (IntPtr)(long)num3), ref Unsafe.Add(ref source2, (IntPtr)(long)num3), num4);
			}
			return;
		}
		goto IL_00da;
		IL_00da:
		bool num5 = ((sizeof(IntPtr) == 4) ? ((uint)(int)intPtr3 > (uint)(-(int)intPtr2)) : ((ulong)(long)intPtr3 > (ulong)(-(long)intPtr2)));
		int num6 = (num5 ? 1 : (-1));
		int num7 = ((!num5) ? (srcLength - 1) : 0);
		int i;
		for (i = 0; i < (srcLength & -8); i += 8)
		{
			Unsafe.Add(ref dst, num7) = Unsafe.Add(ref src, num7);
			Unsafe.Add(ref dst, num7 + num6) = Unsafe.Add(ref src, num7 + num6);
			Unsafe.Add(ref dst, num7 + num6 * 2) = Unsafe.Add(ref src, num7 + num6 * 2);
			Unsafe.Add(ref dst, num7 + num6 * 3) = Unsafe.Add(ref src, num7 + num6 * 3);
			Unsafe.Add(ref dst, num7 + num6 * 4) = Unsafe.Add(ref src, num7 + num6 * 4);
			Unsafe.Add(ref dst, num7 + num6 * 5) = Unsafe.Add(ref src, num7 + num6 * 5);
			Unsafe.Add(ref dst, num7 + num6 * 6) = Unsafe.Add(ref src, num7 + num6 * 6);
			Unsafe.Add(ref dst, num7 + num6 * 7) = Unsafe.Add(ref src, num7 + num6 * 7);
			num7 += num6 * 8;
		}
		if (i < (srcLength & -4))
		{
			Unsafe.Add(ref dst, num7) = Unsafe.Add(ref src, num7);
			Unsafe.Add(ref dst, num7 + num6) = Unsafe.Add(ref src, num7 + num6);
			Unsafe.Add(ref dst, num7 + num6 * 2) = Unsafe.Add(ref src, num7 + num6 * 2);
			Unsafe.Add(ref dst, num7 + num6 * 3) = Unsafe.Add(ref src, num7 + num6 * 3);
			num7 += num6 * 4;
			i += 4;
		}
		for (; i < srcLength; i++)
		{
			Unsafe.Add(ref dst, num7) = Unsafe.Add(ref src, num7);
			num7 += num6;
		}
	}

	[MethodImpl(MethodImplOptions.AggressiveInlining)]
	public unsafe static IntPtr Add<T>(this IntPtr start, int index)
	{
		if (sizeof(IntPtr) == 4)
		{
			uint num = (uint)(index * Unsafe.SizeOf<T>());
			return (IntPtr)((byte*)(void*)start + num);
		}
		ulong num2 = (ulong)index * (ulong)Unsafe.SizeOf<T>();
		return (IntPtr)((byte*)(void*)start + num2);
	}

	public static bool IsReferenceOrContainsReferences<T>()
	{
		return PerTypeValues<T>.IsReferenceOrContainsReferences;
	}

	private static bool IsReferenceOrContainsReferencesCore(Type type)
	{
		if (type.GetTypeInfo().IsPrimitive)
		{
			return false;
		}
		if (!type.GetTypeInfo().IsValueType)
		{
			return true;
		}
		Type underlyingType = Nullable.GetUnderlyingType(type);
		if (underlyingType != null)
		{
			type = underlyingType;
		}
		if (type.GetTypeInfo().IsEnum)
		{
			return false;
		}
		foreach (FieldInfo declaredField in type.GetTypeInfo().DeclaredFields)
		{
			if (!declaredField.IsStatic && IsReferenceOrContainsReferencesCore(declaredField.FieldType))
			{
				return true;
			}
		}
		return false;
	}
}
