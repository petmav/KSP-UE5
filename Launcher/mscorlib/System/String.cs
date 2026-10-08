using System.Collections;
using System.Collections.Generic;
using System.Globalization;
using System.Runtime.CompilerServices;
using System.Runtime.ConstrainedExecution;
using System.Runtime.InteropServices;
using System.Security;
using System.Text;

namespace System;

[Serializable]
[ComVisible(true)]
public sealed class String : IComparable, ICloneable, IConvertible, IEnumerable, IComparable<string>, IEnumerable<char>, IEquatable<string>
{
	[NonSerialized]
	private int m_stringLength;

	[NonSerialized]
	private char m_firstChar;

	private const int TrimHead = 0;

	private const int TrimTail = 1;

	private const int TrimBoth = 2;

	public static readonly string Empty;

	private const int charPtrAlignConst = 1;

	private const int alignConst = 3;

	internal char FirstChar => m_firstChar;

	[IndexerName("Chars")]
	public unsafe char this[int index]
	{
		get
		{
			if (index < 0 || index >= m_stringLength)
			{
				throw new IndexOutOfRangeException();
			}
			fixed (char* ptr = &m_firstChar)
			{
				return ptr[index];
			}
		}
	}

	public int Length => m_stringLength;

	public static string Join(string separator, params string[] value)
	{
		if (value == null)
		{
			throw new ArgumentNullException("value");
		}
		return Join(separator, value, 0, value.Length);
	}

	[ComVisible(false)]
	public static string Join(string separator, params object[] values)
	{
		if (values == null)
		{
			throw new ArgumentNullException("values");
		}
		if (values.Length == 0 || values[0] == null)
		{
			return Empty;
		}
		if ((object)separator == null)
		{
			separator = Empty;
		}
		StringBuilder stringBuilder = StringBuilderCache.Acquire();
		string text = values[0].ToString();
		if ((object)text != null)
		{
			stringBuilder.Append(text);
		}
		for (int i = 1; i < values.Length; i++)
		{
			stringBuilder.Append(separator);
			if (values[i] != null)
			{
				text = values[i].ToString();
				if ((object)text != null)
				{
					stringBuilder.Append(text);
				}
			}
		}
		return StringBuilderCache.GetStringAndRelease(stringBuilder);
	}

	[ComVisible(false)]
	public static string Join<T>(string separator, IEnumerable<T> values)
	{
		if (values == null)
		{
			throw new ArgumentNullException("values");
		}
		if ((object)separator == null)
		{
			separator = Empty;
		}
		using IEnumerator<T> enumerator = values.GetEnumerator();
		if (!enumerator.MoveNext())
		{
			return Empty;
		}
		StringBuilder stringBuilder = StringBuilderCache.Acquire();
		if (enumerator.Current != null)
		{
			string text = enumerator.Current.ToString();
			if ((object)text != null)
			{
				stringBuilder.Append(text);
			}
		}
		while (enumerator.MoveNext())
		{
			stringBuilder.Append(separator);
			if (enumerator.Current != null)
			{
				string text2 = enumerator.Current.ToString();
				if ((object)text2 != null)
				{
					stringBuilder.Append(text2);
				}
			}
		}
		return StringBuilderCache.GetStringAndRelease(stringBuilder);
	}

	[ComVisible(false)]
	public static string Join(string separator, IEnumerable<string> values)
	{
		if (values == null)
		{
			throw new ArgumentNullException("values");
		}
		if ((object)separator == null)
		{
			separator = Empty;
		}
		using IEnumerator<string> enumerator = values.GetEnumerator();
		if (!enumerator.MoveNext())
		{
			return Empty;
		}
		StringBuilder stringBuilder = StringBuilderCache.Acquire();
		if ((object)enumerator.Current != null)
		{
			stringBuilder.Append(enumerator.Current);
		}
		while (enumerator.MoveNext())
		{
			stringBuilder.Append(separator);
			if ((object)enumerator.Current != null)
			{
				stringBuilder.Append(enumerator.Current);
			}
		}
		return StringBuilderCache.GetStringAndRelease(stringBuilder);
	}

	[SecuritySafeCritical]
	public unsafe static string Join(string separator, string[] value, int startIndex, int count)
	{
		if (value == null)
		{
			throw new ArgumentNullException("value");
		}
		if (startIndex < 0)
		{
			throw new ArgumentOutOfRangeException("startIndex", Environment.GetResourceString("StartIndex cannot be less than zero."));
		}
		if (count < 0)
		{
			throw new ArgumentOutOfRangeException("count", Environment.GetResourceString("Count cannot be less than zero."));
		}
		if (startIndex > value.Length - count)
		{
			throw new ArgumentOutOfRangeException("startIndex", Environment.GetResourceString("Index and count must refer to a location within the buffer."));
		}
		if ((object)separator == null)
		{
			separator = Empty;
		}
		if (count == 0)
		{
			return Empty;
		}
		int num = 0;
		int num2 = startIndex + count - 1;
		for (int i = startIndex; i <= num2; i++)
		{
			if ((object)value[i] != null)
			{
				num += value[i].Length;
			}
		}
		num += (count - 1) * separator.Length;
		if (num < 0 || num + 1 < 0)
		{
			throw new OutOfMemoryException();
		}
		if (num == 0)
		{
			return Empty;
		}
		string text = FastAllocateString(num);
		fixed (char* buffer = &text.m_firstChar)
		{
			UnSafeCharBuffer unSafeCharBuffer = new UnSafeCharBuffer(buffer, num);
			unSafeCharBuffer.AppendString(value[startIndex]);
			for (int j = startIndex + 1; j <= num2; j++)
			{
				unSafeCharBuffer.AppendString(separator);
				unSafeCharBuffer.AppendString(value[j]);
			}
		}
		return text;
	}

	[SecuritySafeCritical]
	private unsafe static int CompareOrdinalIgnoreCaseHelper(string strA, string strB)
	{
		int num = Math.Min(strA.Length, strB.Length);
		fixed (char* ptr = &strA.m_firstChar)
		{
			fixed (char* ptr3 = &strB.m_firstChar)
			{
				char* ptr2 = ptr;
				char* ptr4 = ptr3;
				while (num != 0)
				{
					int num2 = *ptr2;
					int num3 = *ptr4;
					if ((uint)(num2 - 97) <= 25u)
					{
						num2 -= 32;
					}
					if ((uint)(num3 - 97) <= 25u)
					{
						num3 -= 32;
					}
					if (num2 != num3)
					{
						return num2 - num3;
					}
					ptr2++;
					ptr4++;
					num--;
				}
				return strA.Length - strB.Length;
			}
		}
	}

	[SecuritySafeCritical]
	internal unsafe static string SmallCharToUpper(string strIn)
	{
		int length = strIn.Length;
		string text = FastAllocateString(length);
		fixed (char* ptr = &strIn.m_firstChar)
		{
			fixed (char* ptr2 = &text.m_firstChar)
			{
				for (int i = 0; i < length; i++)
				{
					int num = ptr[i];
					if ((uint)(num - 97) <= 25u)
					{
						num -= 32;
					}
					ptr2[i] = (char)num;
				}
			}
		}
		return text;
	}

	[SecuritySafeCritical]
	[ReliabilityContract(Consistency.WillNotCorruptState, Cer.MayFail)]
	private unsafe static bool EqualsHelper(string strA, string strB)
	{
		int num = strA.Length;
		fixed (char* ptr = &strA.m_firstChar)
		{
			fixed (char* ptr3 = &strB.m_firstChar)
			{
				char* ptr2 = ptr;
				char* ptr4 = ptr3;
				if (Environment.Is64BitProcess)
				{
					while (num >= 12)
					{
						if (*(long*)ptr2 != *(long*)ptr4)
						{
							return false;
						}
						if (*(long*)(ptr2 + 4) != *(long*)(ptr4 + 4))
						{
							return false;
						}
						if (*(long*)(ptr2 + 8) != *(long*)(ptr4 + 8))
						{
							return false;
						}
						ptr2 += 12;
						ptr4 += 12;
						num -= 12;
					}
				}
				else
				{
					while (num >= 10)
					{
						if (*(int*)ptr2 != *(int*)ptr4)
						{
							return false;
						}
						if (*(int*)(ptr2 + 2) != *(int*)(ptr4 + 2))
						{
							return false;
						}
						if (*(int*)(ptr2 + 4) != *(int*)(ptr4 + 4))
						{
							return false;
						}
						if (*(int*)(ptr2 + 6) != *(int*)(ptr4 + 6))
						{
							return false;
						}
						if (*(int*)(ptr2 + 8) != *(int*)(ptr4 + 8))
						{
							return false;
						}
						ptr2 += 10;
						ptr4 += 10;
						num -= 10;
					}
				}
				while (num > 0 && *(int*)ptr2 == *(int*)ptr4)
				{
					ptr2 += 2;
					ptr4 += 2;
					num -= 2;
				}
				return num <= 0;
			}
		}
	}

	[SecuritySafeCritical]
	private unsafe static int CompareOrdinalHelper(string strA, string strB)
	{
		int num = Math.Min(strA.Length, strB.Length);
		int num2 = -1;
		fixed (char* ptr = &strA.m_firstChar)
		{
			fixed (char* ptr3 = &strB.m_firstChar)
			{
				char* ptr2 = ptr;
				char* ptr4 = ptr3;
				while (num >= 10)
				{
					if (*(int*)ptr2 != *(int*)ptr4)
					{
						num2 = 0;
						break;
					}
					if (*(int*)(ptr2 + 2) != *(int*)(ptr4 + 2))
					{
						num2 = 2;
						break;
					}
					if (*(int*)(ptr2 + 4) != *(int*)(ptr4 + 4))
					{
						num2 = 4;
						break;
					}
					if (*(int*)(ptr2 + 6) != *(int*)(ptr4 + 6))
					{
						num2 = 6;
						break;
					}
					if (*(int*)(ptr2 + 8) != *(int*)(ptr4 + 8))
					{
						num2 = 8;
						break;
					}
					ptr2 += 10;
					ptr4 += 10;
					num -= 10;
				}
				if (num2 != -1)
				{
					ptr2 += num2;
					ptr4 += num2;
					int result;
					if ((result = *ptr2 - *ptr4) != 0)
					{
						return result;
					}
					return ptr2[1] - ptr4[1];
				}
				while (num > 0 && *(int*)ptr2 == *(int*)ptr4)
				{
					ptr2 += 2;
					ptr4 += 2;
					num -= 2;
				}
				if (num > 0)
				{
					int result2;
					if ((result2 = *ptr2 - *ptr4) != 0)
					{
						return result2;
					}
					return ptr2[1] - ptr4[1];
				}
				return strA.Length - strB.Length;
			}
		}
	}

	[ReliabilityContract(Consistency.WillNotCorruptState, Cer.MayFail)]
	public override bool Equals(object obj)
	{
		if ((object)this == null)
		{
			throw new NullReferenceException();
		}
		if (!(obj is string text))
		{
			return false;
		}
		if (this == obj)
		{
			return true;
		}
		if (Length != text.Length)
		{
			return false;
		}
		return EqualsHelper(this, text);
	}

	[ReliabilityContract(Consistency.WillNotCorruptState, Cer.MayFail)]
	public bool Equals(string value)
	{
		if ((object)this == null)
		{
			throw new NullReferenceException();
		}
		if ((object)value == null)
		{
			return false;
		}
		if ((object)this == value)
		{
			return true;
		}
		if (Length != value.Length)
		{
			return false;
		}
		return EqualsHelper(this, value);
	}

	[SecuritySafeCritical]
	public bool Equals(string value, StringComparison comparisonType)
	{
		if (comparisonType < StringComparison.CurrentCulture || comparisonType > StringComparison.OrdinalIgnoreCase)
		{
			throw new ArgumentException(Environment.GetResourceString("The string comparison type passed in is currently not supported."), "comparisonType");
		}
		if ((object)this == value)
		{
			return true;
		}
		if ((object)value == null)
		{
			return false;
		}
		switch (comparisonType)
		{
		case StringComparison.CurrentCulture:
			return CultureInfo.CurrentCulture.CompareInfo.Compare(this, value, CompareOptions.None) == 0;
		case StringComparison.CurrentCultureIgnoreCase:
			return CultureInfo.CurrentCulture.CompareInfo.Compare(this, value, CompareOptions.IgnoreCase) == 0;
		case StringComparison.InvariantCulture:
			return CultureInfo.InvariantCulture.CompareInfo.Compare(this, value, CompareOptions.None) == 0;
		case StringComparison.InvariantCultureIgnoreCase:
			return CultureInfo.InvariantCulture.CompareInfo.Compare(this, value, CompareOptions.IgnoreCase) == 0;
		case StringComparison.Ordinal:
			if (Length != value.Length)
			{
				return false;
			}
			return EqualsHelper(this, value);
		case StringComparison.OrdinalIgnoreCase:
			if (Length != value.Length)
			{
				return false;
			}
			if (IsAscii() && value.IsAscii())
			{
				return CompareOrdinalIgnoreCaseHelper(this, value) == 0;
			}
			return TextInfo.CompareOrdinalIgnoreCase(this, value) == 0;
		default:
			throw new ArgumentException(Environment.GetResourceString("The string comparison type passed in is currently not supported."), "comparisonType");
		}
	}

	public static bool Equals(string a, string b)
	{
		if ((object)a == b)
		{
			return true;
		}
		if ((object)a == null || (object)b == null)
		{
			return false;
		}
		if (a.Length != b.Length)
		{
			return false;
		}
		return EqualsHelper(a, b);
	}

	[SecuritySafeCritical]
	public static bool Equals(string a, string b, StringComparison comparisonType)
	{
		if (comparisonType < StringComparison.CurrentCulture || comparisonType > StringComparison.OrdinalIgnoreCase)
		{
			throw new ArgumentException(Environment.GetResourceString("The string comparison type passed in is currently not supported."), "comparisonType");
		}
		if ((object)a == b)
		{
			return true;
		}
		if ((object)a == null || (object)b == null)
		{
			return false;
		}
		switch (comparisonType)
		{
		case StringComparison.CurrentCulture:
			return CultureInfo.CurrentCulture.CompareInfo.Compare(a, b, CompareOptions.None) == 0;
		case StringComparison.CurrentCultureIgnoreCase:
			return CultureInfo.CurrentCulture.CompareInfo.Compare(a, b, CompareOptions.IgnoreCase) == 0;
		case StringComparison.InvariantCulture:
			return CultureInfo.InvariantCulture.CompareInfo.Compare(a, b, CompareOptions.None) == 0;
		case StringComparison.InvariantCultureIgnoreCase:
			return CultureInfo.InvariantCulture.CompareInfo.Compare(a, b, CompareOptions.IgnoreCase) == 0;
		case StringComparison.Ordinal:
			if (a.Length != b.Length)
			{
				return false;
			}
			return EqualsHelper(a, b);
		case StringComparison.OrdinalIgnoreCase:
			if (a.Length != b.Length)
			{
				return false;
			}
			if (a.IsAscii() && b.IsAscii())
			{
				return CompareOrdinalIgnoreCaseHelper(a, b) == 0;
			}
			return TextInfo.CompareOrdinalIgnoreCase(a, b) == 0;
		default:
			throw new ArgumentException(Environment.GetResourceString("The string comparison type passed in is currently not supported."), "comparisonType");
		}
	}

	public static bool operator ==(string a, string b)
	{
		return Equals(a, b);
	}

	public static bool operator !=(string a, string b)
	{
		return !Equals(a, b);
	}

	[SecuritySafeCritical]
	public unsafe void CopyTo(int sourceIndex, char[] destination, int destinationIndex, int count)
	{
		if (destination == null)
		{
			throw new ArgumentNullException("destination");
		}
		if (count < 0)
		{
			throw new ArgumentOutOfRangeException("count", Environment.GetResourceString("Count cannot be less than zero."));
		}
		if (sourceIndex < 0)
		{
			throw new ArgumentOutOfRangeException("sourceIndex", Environment.GetResourceString("Index was out of range. Must be non-negative and less than the size of the collection."));
		}
		if (count > Length - sourceIndex)
		{
			throw new ArgumentOutOfRangeException("sourceIndex", Environment.GetResourceString("Index and count must refer to a location within the string."));
		}
		if (destinationIndex > destination.Length - count || destinationIndex < 0)
		{
			throw new ArgumentOutOfRangeException("destinationIndex", Environment.GetResourceString("Index and count must refer to a location within the string."));
		}
		if (count <= 0)
		{
			return;
		}
		fixed (char* ptr2 = &m_firstChar)
		{
			fixed (char* ptr = destination)
			{
				wstrcpy(ptr + destinationIndex, ptr2 + sourceIndex, count);
			}
		}
	}

	[SecuritySafeCritical]
	public unsafe char[] ToCharArray()
	{
		int length = Length;
		char[] array = new char[length];
		if (length > 0)
		{
			fixed (char* smem = &m_firstChar)
			{
				fixed (char* dmem = array)
				{
					wstrcpy(dmem, smem, length);
				}
			}
		}
		return array;
	}

	[SecuritySafeCritical]
	public unsafe char[] ToCharArray(int startIndex, int length)
	{
		if (startIndex < 0 || startIndex > Length || startIndex > Length - length)
		{
			throw new ArgumentOutOfRangeException("startIndex", Environment.GetResourceString("Index was out of range. Must be non-negative and less than the size of the collection."));
		}
		if (length < 0)
		{
			throw new ArgumentOutOfRangeException("length", Environment.GetResourceString("Index was out of range. Must be non-negative and less than the size of the collection."));
		}
		char[] array = new char[length];
		if (length > 0)
		{
			fixed (char* ptr = &m_firstChar)
			{
				fixed (char* dmem = array)
				{
					wstrcpy(dmem, ptr + startIndex, length);
				}
			}
		}
		return array;
	}

	public static bool IsNullOrEmpty(string value)
	{
		if ((object)value != null)
		{
			return value.Length == 0;
		}
		return true;
	}

	public static bool IsNullOrWhiteSpace(string value)
	{
		if ((object)value == null)
		{
			return true;
		}
		for (int i = 0; i < value.Length; i++)
		{
			if (!char.IsWhiteSpace(value[i]))
			{
				return false;
			}
		}
		return true;
	}

	[SecuritySafeCritical]
	[ReliabilityContract(Consistency.WillNotCorruptState, Cer.MayFail)]
	public unsafe override int GetHashCode()
	{
		fixed (char* ptr = this)
		{
			int num = 5381;
			int num2 = num;
			char* ptr2 = ptr;
			int num3;
			while ((num3 = *ptr2) != 0)
			{
				num = ((num << 5) + num) ^ num3;
				num3 = ptr2[1];
				if (num3 == 0)
				{
					break;
				}
				num2 = ((num2 << 5) + num2) ^ num3;
				ptr2 += 2;
			}
			return num + num2 * 1566083941;
		}
	}

	[SecuritySafeCritical]
	[ReliabilityContract(Consistency.WillNotCorruptState, Cer.MayFail)]
	internal unsafe int GetLegacyNonRandomizedHashCode()
	{
		fixed (char* ptr = this)
		{
			int num = 5381;
			int num2 = num;
			char* ptr2 = ptr;
			int num3;
			while ((num3 = *ptr2) != 0)
			{
				num = ((num << 5) + num) ^ num3;
				num3 = ptr2[1];
				if (num3 == 0)
				{
					break;
				}
				num2 = ((num2 << 5) + num2) ^ num3;
				ptr2 += 2;
			}
			return num + num2 * 1566083941;
		}
	}

	public string[] Split(params char[] separator)
	{
		return SplitInternal(separator, int.MaxValue, StringSplitOptions.None);
	}

	public string[] Split(char[] separator, int count)
	{
		return SplitInternal(separator, count, StringSplitOptions.None);
	}

	[ComVisible(false)]
	public string[] Split(char[] separator, StringSplitOptions options)
	{
		return SplitInternal(separator, int.MaxValue, options);
	}

	[ComVisible(false)]
	public string[] Split(char[] separator, int count, StringSplitOptions options)
	{
		return SplitInternal(separator, count, options);
	}

	[ComVisible(false)]
	internal string[] SplitInternal(char[] separator, int count, StringSplitOptions options)
	{
		if (count < 0)
		{
			throw new ArgumentOutOfRangeException("count", Environment.GetResourceString("Count cannot be less than zero."));
		}
		if (options < StringSplitOptions.None || options > StringSplitOptions.RemoveEmptyEntries)
		{
			throw new ArgumentException(Environment.GetResourceString("Illegal enum value: {0}.", options));
		}
		bool flag = options == StringSplitOptions.RemoveEmptyEntries;
		if (count == 0 || (flag && Length == 0))
		{
			return new string[0];
		}
		int[] sepList = new int[Length];
		int num = MakeSeparatorList(separator, ref sepList);
		if (num == 0 || count == 1)
		{
			return new string[1] { this };
		}
		if (flag)
		{
			return InternalSplitOmitEmptyEntries(sepList, null, num, count);
		}
		return InternalSplitKeepEmptyEntries(sepList, null, num, count);
	}

	[ComVisible(false)]
	public string[] Split(string[] separator, StringSplitOptions options)
	{
		return Split(separator, int.MaxValue, options);
	}

	[ComVisible(false)]
	public string[] Split(string[] separator, int count, StringSplitOptions options)
	{
		if (count < 0)
		{
			throw new ArgumentOutOfRangeException("count", Environment.GetResourceString("Count cannot be less than zero."));
		}
		if (options < StringSplitOptions.None || options > StringSplitOptions.RemoveEmptyEntries)
		{
			throw new ArgumentException(Environment.GetResourceString("Illegal enum value: {0}.", (int)options));
		}
		bool flag = options == StringSplitOptions.RemoveEmptyEntries;
		if (separator == null || separator.Length == 0)
		{
			return SplitInternal(null, count, options);
		}
		if (count == 0 || (flag && Length == 0))
		{
			return new string[0];
		}
		int[] sepList = new int[Length];
		int[] lengthList = new int[Length];
		int num = MakeSeparatorList(separator, ref sepList, ref lengthList);
		if (num == 0 || count == 1)
		{
			return new string[1] { this };
		}
		if (flag)
		{
			return InternalSplitOmitEmptyEntries(sepList, lengthList, num, count);
		}
		return InternalSplitKeepEmptyEntries(sepList, lengthList, num, count);
	}

	private string[] InternalSplitKeepEmptyEntries(int[] sepList, int[] lengthList, int numReplaces, int count)
	{
		int num = 0;
		int num2 = 0;
		count--;
		int num3 = ((numReplaces < count) ? numReplaces : count);
		string[] array = new string[num3 + 1];
		for (int i = 0; i < num3; i++)
		{
			if (num >= Length)
			{
				break;
			}
			array[num2++] = Substring(num, sepList[i] - num);
			num = sepList[i] + ((lengthList == null) ? 1 : lengthList[i]);
		}
		if (num < Length && num3 >= 0)
		{
			array[num2] = Substring(num);
		}
		else if (num2 == num3)
		{
			array[num2] = Empty;
		}
		return array;
	}

	private string[] InternalSplitOmitEmptyEntries(int[] sepList, int[] lengthList, int numReplaces, int count)
	{
		int num = ((numReplaces < count) ? (numReplaces + 1) : count);
		string[] array = new string[num];
		int num2 = 0;
		int num3 = 0;
		for (int i = 0; i < numReplaces; i++)
		{
			if (num2 >= Length)
			{
				break;
			}
			if (sepList[i] - num2 > 0)
			{
				array[num3++] = Substring(num2, sepList[i] - num2);
			}
			num2 = sepList[i] + ((lengthList == null) ? 1 : lengthList[i]);
			if (num3 == count - 1)
			{
				while (i < numReplaces - 1 && num2 == sepList[++i])
				{
					num2 += ((lengthList == null) ? 1 : lengthList[i]);
				}
				break;
			}
		}
		if (num2 < Length)
		{
			array[num3++] = Substring(num2);
		}
		string[] array2 = array;
		if (num3 != num)
		{
			array2 = new string[num3];
			for (int j = 0; j < num3; j++)
			{
				array2[j] = array[j];
			}
		}
		return array2;
	}

	[SecuritySafeCritical]
	private unsafe int MakeSeparatorList(char[] separator, ref int[] sepList)
	{
		int num = 0;
		if (separator == null || separator.Length == 0)
		{
			fixed (char* ptr = &m_firstChar)
			{
				for (int i = 0; i < Length; i++)
				{
					if (num >= sepList.Length)
					{
						break;
					}
					if (char.IsWhiteSpace(ptr[i]))
					{
						sepList[num++] = i;
					}
				}
			}
		}
		else
		{
			int num2 = sepList.Length;
			int num3 = separator.Length;
			fixed (char* ptr4 = &m_firstChar)
			{
				fixed (char* ptr2 = separator)
				{
					for (int j = 0; j < Length; j++)
					{
						if (num >= num2)
						{
							break;
						}
						char* ptr3 = ptr2;
						int num4 = 0;
						while (num4 < num3)
						{
							if (ptr4[j] == *ptr3)
							{
								sepList[num++] = j;
								break;
							}
							num4++;
							ptr3++;
						}
					}
				}
			}
		}
		return num;
	}

	[SecuritySafeCritical]
	private unsafe int MakeSeparatorList(string[] separators, ref int[] sepList, ref int[] lengthList)
	{
		int num = 0;
		int num2 = sepList.Length;
		_ = separators.Length;
		fixed (char* ptr = &m_firstChar)
		{
			for (int i = 0; i < Length; i++)
			{
				if (num >= num2)
				{
					break;
				}
				foreach (string text in separators)
				{
					if (!IsNullOrEmpty(text))
					{
						int length = text.Length;
						if (ptr[i] == text[0] && length <= Length - i && (length == 1 || CompareOrdinal(this, i, text, 0, length) == 0))
						{
							sepList[num] = i;
							lengthList[num] = length;
							num++;
							i += length - 1;
							break;
						}
					}
				}
			}
		}
		return num;
	}

	public string Substring(int startIndex)
	{
		return Substring(startIndex, Length - startIndex);
	}

	[SecuritySafeCritical]
	public string Substring(int startIndex, int length)
	{
		if (startIndex < 0)
		{
			throw new ArgumentOutOfRangeException("startIndex", Environment.GetResourceString("StartIndex cannot be less than zero."));
		}
		if (startIndex > Length)
		{
			throw new ArgumentOutOfRangeException("startIndex", Environment.GetResourceString("startIndex cannot be larger than length of string."));
		}
		if (length < 0)
		{
			throw new ArgumentOutOfRangeException("length", Environment.GetResourceString("Length cannot be less than zero."));
		}
		if (startIndex > Length - length)
		{
			throw new ArgumentOutOfRangeException("length", Environment.GetResourceString("Index and length must refer to a location within the string."));
		}
		if (length == 0)
		{
			return Empty;
		}
		if (startIndex == 0 && length == Length)
		{
			return this;
		}
		return InternalSubString(startIndex, length);
	}

	[SecurityCritical]
	private unsafe string InternalSubString(int startIndex, int length)
	{
		string text = FastAllocateString(length);
		fixed (char* dmem = &text.m_firstChar)
		{
			fixed (char* ptr = &m_firstChar)
			{
				wstrcpy(dmem, ptr + startIndex, length);
			}
		}
		return text;
	}

	public string Trim(params char[] trimChars)
	{
		if (trimChars == null || trimChars.Length == 0)
		{
			return TrimHelper(2);
		}
		return TrimHelper(trimChars, 2);
	}

	public string TrimStart(params char[] trimChars)
	{
		if (trimChars == null || trimChars.Length == 0)
		{
			return TrimHelper(0);
		}
		return TrimHelper(trimChars, 0);
	}

	public string TrimEnd(params char[] trimChars)
	{
		if (trimChars == null || trimChars.Length == 0)
		{
			return TrimHelper(1);
		}
		return TrimHelper(trimChars, 1);
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	[CLSCompliant(false)]
	[SecurityCritical]
	public unsafe extern String(char* value);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[CLSCompliant(false)]
	[SecurityCritical]
	public unsafe extern String(char* value, int startIndex, int length);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[SecurityCritical]
	[CLSCompliant(false)]
	public unsafe extern String(sbyte* value);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[SecurityCritical]
	[CLSCompliant(false)]
	public unsafe extern String(sbyte* value, int startIndex, int length);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[CLSCompliant(false)]
	[SecurityCritical]
	public unsafe extern String(sbyte* value, int startIndex, int length, Encoding enc);

	[SecurityCritical]
	internal unsafe static string CreateStringFromEncoding(byte* bytes, int byteLength, Encoding encoding)
	{
		int charCount = encoding.GetCharCount(bytes, byteLength, null);
		if (charCount == 0)
		{
			return Empty;
		}
		string text = FastAllocateString(charCount);
		fixed (char* chars = &text.m_firstChar)
		{
			encoding.GetChars(bytes, byteLength, chars, charCount, null);
		}
		return text;
	}

	internal unsafe int GetBytesFromEncoding(byte* pbNativeBuffer, int cbNativeBuffer, Encoding encoding)
	{
		fixed (char* chars = &m_firstChar)
		{
			return encoding.GetBytes(chars, m_stringLength, pbNativeBuffer, cbNativeBuffer);
		}
	}

	public bool IsNormalized()
	{
		return IsNormalized(NormalizationForm.FormC);
	}

	[SecuritySafeCritical]
	public bool IsNormalized(NormalizationForm normalizationForm)
	{
		if (IsFastSort() && (normalizationForm == NormalizationForm.FormC || normalizationForm == NormalizationForm.FormKC || normalizationForm == NormalizationForm.FormD || normalizationForm == NormalizationForm.FormKD))
		{
			return true;
		}
		return Normalization.IsNormalized(this, normalizationForm);
	}

	public string Normalize()
	{
		return Normalize(NormalizationForm.FormC);
	}

	[SecuritySafeCritical]
	public string Normalize(NormalizationForm normalizationForm)
	{
		if (IsAscii() && (normalizationForm == NormalizationForm.FormC || normalizationForm == NormalizationForm.FormKC || normalizationForm == NormalizationForm.FormD || normalizationForm == NormalizationForm.FormKD))
		{
			return this;
		}
		return Normalization.Normalize(this, normalizationForm);
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	[SecurityCritical]
	internal static extern string FastAllocateString(int length);

	[SecuritySafeCritical]
	private unsafe static void FillStringChecked(string dest, int destPos, string src)
	{
		if (src.Length > dest.Length - destPos)
		{
			throw new IndexOutOfRangeException();
		}
		fixed (char* ptr = &dest.m_firstChar)
		{
			fixed (char* smem = &src.m_firstChar)
			{
				wstrcpy(ptr + destPos, smem, src.Length);
			}
		}
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	[SecuritySafeCritical]
	public extern String(char[] value, int startIndex, int length);

	[MethodImpl(MethodImplOptions.InternalCall)]
	[SecuritySafeCritical]
	public extern String(char[] value);

	[SecurityCritical]
	internal unsafe static void wstrcpy(char* dmem, char* smem, int charCount)
	{
		Buffer.Memcpy((byte*)dmem, (byte*)smem, charCount * 2);
	}

	[SecuritySafeCritical]
	private unsafe string CtorCharArray(char[] value)
	{
		if (value != null && value.Length != 0)
		{
			string text = FastAllocateString(value.Length);
			fixed (char* dmem = text)
			{
				fixed (char* smem = value)
				{
					wstrcpy(dmem, smem, value.Length);
				}
			}
			return text;
		}
		return Empty;
	}

	[SecuritySafeCritical]
	private unsafe string CtorCharArrayStartLength(char[] value, int startIndex, int length)
	{
		if (value == null)
		{
			throw new ArgumentNullException("value");
		}
		if (startIndex < 0)
		{
			throw new ArgumentOutOfRangeException("startIndex", Environment.GetResourceString("StartIndex cannot be less than zero."));
		}
		if (length < 0)
		{
			throw new ArgumentOutOfRangeException("length", Environment.GetResourceString("Length cannot be less than zero."));
		}
		if (startIndex > value.Length - length)
		{
			throw new ArgumentOutOfRangeException("startIndex", Environment.GetResourceString("Index was out of range. Must be non-negative and less than the size of the collection."));
		}
		if (length > 0)
		{
			string text = FastAllocateString(length);
			fixed (char* dmem = text)
			{
				fixed (char* ptr = value)
				{
					wstrcpy(dmem, ptr + startIndex, length);
				}
			}
			return text;
		}
		return Empty;
	}

	[SecuritySafeCritical]
	private unsafe string CtorCharCount(char c, int count)
	{
		if (count > 0)
		{
			string text = FastAllocateString(count);
			if (c != 0)
			{
				fixed (char* ptr = text)
				{
					char* ptr2 = ptr;
					while (((uint)(int)ptr2 & 3u) != 0 && count > 0)
					{
						*(ptr2++) = c;
						count--;
					}
					uint num = ((uint)c << 16) | c;
					if (count >= 4)
					{
						count -= 4;
						do
						{
							*(uint*)ptr2 = num;
							*(uint*)(ptr2 + 2) = num;
							ptr2 += 4;
							count -= 4;
						}
						while (count >= 0);
					}
					if (((uint)count & 2u) != 0)
					{
						*(uint*)ptr2 = num;
						ptr2 += 2;
					}
					if (((uint)count & (true ? 1u : 0u)) != 0)
					{
						*ptr2 = c;
					}
				}
			}
			return text;
		}
		if (count == 0)
		{
			return Empty;
		}
		throw new ArgumentOutOfRangeException("count", Environment.GetResourceString("'{0}' must be non-negative.", "count"));
	}

	[SecurityCritical]
	private unsafe static int wcslen(char* ptr)
	{
		char* ptr2;
		for (ptr2 = ptr; ((uint)(int)ptr2 & 3u) != 0 && *ptr2 != 0; ptr2++)
		{
		}
		if (*ptr2 != 0)
		{
			for (; (*ptr2 & ptr2[1]) != 0 || (*ptr2 != 0 && ptr2[1] != 0); ptr2 += 2)
			{
			}
		}
		for (; *ptr2 != 0; ptr2++)
		{
		}
		return (int)(ptr2 - ptr);
	}

	[SecurityCritical]
	private unsafe string CtorCharPtr(char* ptr)
	{
		if (ptr == null)
		{
			return Empty;
		}
		try
		{
			int num = wcslen(ptr);
			if (num == 0)
			{
				return Empty;
			}
			string text = FastAllocateString(num);
			fixed (char* dmem = text)
			{
				wstrcpy(dmem, ptr, num);
			}
			return text;
		}
		catch (NullReferenceException)
		{
			throw new ArgumentOutOfRangeException("ptr", Environment.GetResourceString("Pointer startIndex and length do not refer to a valid string."));
		}
	}

	[SecurityCritical]
	private unsafe string CtorCharPtrStartLength(char* ptr, int startIndex, int length)
	{
		if (length < 0)
		{
			throw new ArgumentOutOfRangeException("length", Environment.GetResourceString("Length cannot be less than zero."));
		}
		if (startIndex < 0)
		{
			throw new ArgumentOutOfRangeException("startIndex", Environment.GetResourceString("StartIndex cannot be less than zero."));
		}
		char* ptr2 = ptr + startIndex;
		if (ptr2 < ptr)
		{
			throw new ArgumentOutOfRangeException("startIndex", Environment.GetResourceString("Pointer startIndex and length do not refer to a valid string."));
		}
		if (length == 0)
		{
			return Empty;
		}
		string text = FastAllocateString(length);
		try
		{
			fixed (char* dmem = text)
			{
				wstrcpy(dmem, ptr2, length);
			}
			return text;
		}
		catch (NullReferenceException)
		{
			throw new ArgumentOutOfRangeException("ptr", Environment.GetResourceString("Pointer startIndex and length do not refer to a valid string."));
		}
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	[SecuritySafeCritical]
	public extern String(char c, int count);

	public static int Compare(string strA, string strB)
	{
		return CultureInfo.CurrentCulture.CompareInfo.Compare(strA, strB, CompareOptions.None);
	}

	public static int Compare(string strA, string strB, bool ignoreCase)
	{
		if (ignoreCase)
		{
			return CultureInfo.CurrentCulture.CompareInfo.Compare(strA, strB, CompareOptions.IgnoreCase);
		}
		return CultureInfo.CurrentCulture.CompareInfo.Compare(strA, strB, CompareOptions.None);
	}

	[SecuritySafeCritical]
	public static int Compare(string strA, string strB, StringComparison comparisonType)
	{
		if ((uint)(comparisonType - 0) > 5u)
		{
			throw new ArgumentException(Environment.GetResourceString("The string comparison type passed in is currently not supported."), "comparisonType");
		}
		if ((object)strA == strB)
		{
			return 0;
		}
		if ((object)strA == null)
		{
			return -1;
		}
		if ((object)strB == null)
		{
			return 1;
		}
		switch (comparisonType)
		{
		case StringComparison.CurrentCulture:
			return CultureInfo.CurrentCulture.CompareInfo.Compare(strA, strB, CompareOptions.None);
		case StringComparison.CurrentCultureIgnoreCase:
			return CultureInfo.CurrentCulture.CompareInfo.Compare(strA, strB, CompareOptions.IgnoreCase);
		case StringComparison.InvariantCulture:
			return CultureInfo.InvariantCulture.CompareInfo.Compare(strA, strB, CompareOptions.None);
		case StringComparison.InvariantCultureIgnoreCase:
			return CultureInfo.InvariantCulture.CompareInfo.Compare(strA, strB, CompareOptions.IgnoreCase);
		case StringComparison.Ordinal:
			if (strA.m_firstChar - strB.m_firstChar != 0)
			{
				return strA.m_firstChar - strB.m_firstChar;
			}
			return CompareOrdinalHelper(strA, strB);
		case StringComparison.OrdinalIgnoreCase:
			if (strA.IsAscii() && strB.IsAscii())
			{
				return CompareOrdinalIgnoreCaseHelper(strA, strB);
			}
			return TextInfo.CompareOrdinalIgnoreCase(strA, strB);
		default:
			throw new NotSupportedException(Environment.GetResourceString("The string comparison type passed in is currently not supported."));
		}
	}

	public static int Compare(string strA, string strB, CultureInfo culture, CompareOptions options)
	{
		if (culture == null)
		{
			throw new ArgumentNullException("culture");
		}
		return culture.CompareInfo.Compare(strA, strB, options);
	}

	public static int Compare(string strA, string strB, bool ignoreCase, CultureInfo culture)
	{
		if (culture == null)
		{
			throw new ArgumentNullException("culture");
		}
		if (ignoreCase)
		{
			return culture.CompareInfo.Compare(strA, strB, CompareOptions.IgnoreCase);
		}
		return culture.CompareInfo.Compare(strA, strB, CompareOptions.None);
	}

	public static int Compare(string strA, int indexA, string strB, int indexB, int length)
	{
		int num = length;
		int num2 = length;
		if ((object)strA != null && strA.Length - indexA < num)
		{
			num = strA.Length - indexA;
		}
		if ((object)strB != null && strB.Length - indexB < num2)
		{
			num2 = strB.Length - indexB;
		}
		return CultureInfo.CurrentCulture.CompareInfo.Compare(strA, indexA, num, strB, indexB, num2, CompareOptions.None);
	}

	public static int Compare(string strA, int indexA, string strB, int indexB, int length, bool ignoreCase)
	{
		int num = length;
		int num2 = length;
		if ((object)strA != null && strA.Length - indexA < num)
		{
			num = strA.Length - indexA;
		}
		if ((object)strB != null && strB.Length - indexB < num2)
		{
			num2 = strB.Length - indexB;
		}
		if (ignoreCase)
		{
			return CultureInfo.CurrentCulture.CompareInfo.Compare(strA, indexA, num, strB, indexB, num2, CompareOptions.IgnoreCase);
		}
		return CultureInfo.CurrentCulture.CompareInfo.Compare(strA, indexA, num, strB, indexB, num2, CompareOptions.None);
	}

	public static int Compare(string strA, int indexA, string strB, int indexB, int length, bool ignoreCase, CultureInfo culture)
	{
		if (culture == null)
		{
			throw new ArgumentNullException("culture");
		}
		int num = length;
		int num2 = length;
		if ((object)strA != null && strA.Length - indexA < num)
		{
			num = strA.Length - indexA;
		}
		if ((object)strB != null && strB.Length - indexB < num2)
		{
			num2 = strB.Length - indexB;
		}
		if (ignoreCase)
		{
			return culture.CompareInfo.Compare(strA, indexA, num, strB, indexB, num2, CompareOptions.IgnoreCase);
		}
		return culture.CompareInfo.Compare(strA, indexA, num, strB, indexB, num2, CompareOptions.None);
	}

	public static int Compare(string strA, int indexA, string strB, int indexB, int length, CultureInfo culture, CompareOptions options)
	{
		if (culture == null)
		{
			throw new ArgumentNullException("culture");
		}
		int num = length;
		int num2 = length;
		if ((object)strA != null && strA.Length - indexA < num)
		{
			num = strA.Length - indexA;
		}
		if ((object)strB != null && strB.Length - indexB < num2)
		{
			num2 = strB.Length - indexB;
		}
		return culture.CompareInfo.Compare(strA, indexA, num, strB, indexB, num2, options);
	}

	[SecuritySafeCritical]
	public static int Compare(string strA, int indexA, string strB, int indexB, int length, StringComparison comparisonType)
	{
		if (comparisonType < StringComparison.CurrentCulture || comparisonType > StringComparison.OrdinalIgnoreCase)
		{
			throw new ArgumentException(Environment.GetResourceString("The string comparison type passed in is currently not supported."), "comparisonType");
		}
		if ((object)strA == null || (object)strB == null)
		{
			if ((object)strA == strB)
			{
				return 0;
			}
			if ((object)strA != null)
			{
				return 1;
			}
			return -1;
		}
		if (length < 0)
		{
			throw new ArgumentOutOfRangeException("length", Environment.GetResourceString("Length cannot be less than zero."));
		}
		if (indexA < 0)
		{
			throw new ArgumentOutOfRangeException("indexA", Environment.GetResourceString("Index was out of range. Must be non-negative and less than the size of the collection."));
		}
		if (indexB < 0)
		{
			throw new ArgumentOutOfRangeException("indexB", Environment.GetResourceString("Index was out of range. Must be non-negative and less than the size of the collection."));
		}
		if (strA.Length - indexA < 0)
		{
			throw new ArgumentOutOfRangeException("indexA", Environment.GetResourceString("Index was out of range. Must be non-negative and less than the size of the collection."));
		}
		if (strB.Length - indexB < 0)
		{
			throw new ArgumentOutOfRangeException("indexB", Environment.GetResourceString("Index was out of range. Must be non-negative and less than the size of the collection."));
		}
		if (length == 0 || (strA == strB && indexA == indexB))
		{
			return 0;
		}
		int num = length;
		int num2 = length;
		if ((object)strA != null && strA.Length - indexA < num)
		{
			num = strA.Length - indexA;
		}
		if ((object)strB != null && strB.Length - indexB < num2)
		{
			num2 = strB.Length - indexB;
		}
		return comparisonType switch
		{
			StringComparison.CurrentCulture => CultureInfo.CurrentCulture.CompareInfo.Compare(strA, indexA, num, strB, indexB, num2, CompareOptions.None), 
			StringComparison.CurrentCultureIgnoreCase => CultureInfo.CurrentCulture.CompareInfo.Compare(strA, indexA, num, strB, indexB, num2, CompareOptions.IgnoreCase), 
			StringComparison.InvariantCulture => CultureInfo.InvariantCulture.CompareInfo.Compare(strA, indexA, num, strB, indexB, num2, CompareOptions.None), 
			StringComparison.InvariantCultureIgnoreCase => CultureInfo.InvariantCulture.CompareInfo.Compare(strA, indexA, num, strB, indexB, num2, CompareOptions.IgnoreCase), 
			StringComparison.Ordinal => nativeCompareOrdinalEx(strA, indexA, strB, indexB, length), 
			StringComparison.OrdinalIgnoreCase => TextInfo.CompareOrdinalIgnoreCaseEx(strA, indexA, strB, indexB, num, num2), 
			_ => throw new ArgumentException(Environment.GetResourceString("The string comparison type passed in is currently not supported.")), 
		};
	}

	public int CompareTo(object value)
	{
		if (value == null)
		{
			return 1;
		}
		if (!(value is string))
		{
			throw new ArgumentException(Environment.GetResourceString("Object must be of type String."));
		}
		return Compare(this, (string)value, StringComparison.CurrentCulture);
	}

	public int CompareTo(string strB)
	{
		if ((object)strB == null)
		{
			return 1;
		}
		return CultureInfo.CurrentCulture.CompareInfo.Compare(this, strB, CompareOptions.None);
	}

	public static int CompareOrdinal(string strA, string strB)
	{
		if ((object)strA == strB)
		{
			return 0;
		}
		if ((object)strA == null)
		{
			return -1;
		}
		if ((object)strB == null)
		{
			return 1;
		}
		if (strA.m_firstChar - strB.m_firstChar != 0)
		{
			return strA.m_firstChar - strB.m_firstChar;
		}
		return CompareOrdinalHelper(strA, strB);
	}

	[SecuritySafeCritical]
	public static int CompareOrdinal(string strA, int indexA, string strB, int indexB, int length)
	{
		if ((object)strA == null || (object)strB == null)
		{
			if ((object)strA == strB)
			{
				return 0;
			}
			if ((object)strA != null)
			{
				return 1;
			}
			return -1;
		}
		return nativeCompareOrdinalEx(strA, indexA, strB, indexB, length);
	}

	public bool Contains(string value)
	{
		return IndexOf(value, StringComparison.Ordinal) >= 0;
	}

	public bool EndsWith(string value)
	{
		return EndsWith(value, StringComparison.CurrentCulture);
	}

	[ComVisible(false)]
	[SecuritySafeCritical]
	public bool EndsWith(string value, StringComparison comparisonType)
	{
		if ((object)value == null)
		{
			throw new ArgumentNullException("value");
		}
		if (comparisonType < StringComparison.CurrentCulture || comparisonType > StringComparison.OrdinalIgnoreCase)
		{
			throw new ArgumentException(Environment.GetResourceString("The string comparison type passed in is currently not supported."), "comparisonType");
		}
		if ((object)this == value)
		{
			return true;
		}
		if (value.Length == 0)
		{
			return true;
		}
		switch (comparisonType)
		{
		case StringComparison.CurrentCulture:
			return CultureInfo.CurrentCulture.CompareInfo.IsSuffix(this, value, CompareOptions.None);
		case StringComparison.CurrentCultureIgnoreCase:
			return CultureInfo.CurrentCulture.CompareInfo.IsSuffix(this, value, CompareOptions.IgnoreCase);
		case StringComparison.InvariantCulture:
			return CultureInfo.InvariantCulture.CompareInfo.IsSuffix(this, value, CompareOptions.None);
		case StringComparison.InvariantCultureIgnoreCase:
			return CultureInfo.InvariantCulture.CompareInfo.IsSuffix(this, value, CompareOptions.IgnoreCase);
		case StringComparison.Ordinal:
			if (Length >= value.Length)
			{
				return nativeCompareOrdinalEx(this, Length - value.Length, value, 0, value.Length) == 0;
			}
			return false;
		case StringComparison.OrdinalIgnoreCase:
			if (Length >= value.Length)
			{
				return TextInfo.CompareOrdinalIgnoreCaseEx(this, Length - value.Length, value, 0, value.Length, value.Length) == 0;
			}
			return false;
		default:
			throw new ArgumentException(Environment.GetResourceString("The string comparison type passed in is currently not supported."), "comparisonType");
		}
	}

	public bool EndsWith(string value, bool ignoreCase, CultureInfo culture)
	{
		if ((object)value == null)
		{
			throw new ArgumentNullException("value");
		}
		if ((object)this == value)
		{
			return true;
		}
		CultureInfo cultureInfo = ((culture != null) ? culture : CultureInfo.CurrentCulture);
		return cultureInfo.CompareInfo.IsSuffix(this, value, ignoreCase ? CompareOptions.IgnoreCase : CompareOptions.None);
	}

	internal bool EndsWith(char value)
	{
		int length = Length;
		if (length != 0 && this[length - 1] == value)
		{
			return true;
		}
		return false;
	}

	public int IndexOf(char value)
	{
		return IndexOf(value, 0, Length);
	}

	public int IndexOf(char value, int startIndex)
	{
		return IndexOf(value, startIndex, Length - startIndex);
	}

	public int IndexOfAny(char[] anyOf)
	{
		return IndexOfAny(anyOf, 0, Length);
	}

	public int IndexOfAny(char[] anyOf, int startIndex)
	{
		return IndexOfAny(anyOf, startIndex, Length - startIndex);
	}

	public int IndexOf(string value)
	{
		return IndexOf(value, StringComparison.CurrentCulture);
	}

	public int IndexOf(string value, int startIndex)
	{
		return IndexOf(value, startIndex, StringComparison.CurrentCulture);
	}

	public int IndexOf(string value, int startIndex, int count)
	{
		if (startIndex < 0 || startIndex > Length)
		{
			throw new ArgumentOutOfRangeException("startIndex", Environment.GetResourceString("Index was out of range. Must be non-negative and less than the size of the collection."));
		}
		if (count < 0 || count > Length - startIndex)
		{
			throw new ArgumentOutOfRangeException("count", Environment.GetResourceString("Count must be positive and count must refer to a location within the string/array/collection."));
		}
		return IndexOf(value, startIndex, count, StringComparison.CurrentCulture);
	}

	public int IndexOf(string value, StringComparison comparisonType)
	{
		return IndexOf(value, 0, Length, comparisonType);
	}

	public int IndexOf(string value, int startIndex, StringComparison comparisonType)
	{
		return IndexOf(value, startIndex, Length - startIndex, comparisonType);
	}

	[SecuritySafeCritical]
	public int IndexOf(string value, int startIndex, int count, StringComparison comparisonType)
	{
		if ((object)value == null)
		{
			throw new ArgumentNullException("value");
		}
		if (startIndex < 0 || startIndex > Length)
		{
			throw new ArgumentOutOfRangeException("startIndex", Environment.GetResourceString("Index was out of range. Must be non-negative and less than the size of the collection."));
		}
		if (count < 0 || startIndex > Length - count)
		{
			throw new ArgumentOutOfRangeException("count", Environment.GetResourceString("Count must be positive and count must refer to a location within the string/array/collection."));
		}
		switch (comparisonType)
		{
		case StringComparison.CurrentCulture:
			return CultureInfo.CurrentCulture.CompareInfo.IndexOf(this, value, startIndex, count, CompareOptions.None);
		case StringComparison.CurrentCultureIgnoreCase:
			return CultureInfo.CurrentCulture.CompareInfo.IndexOf(this, value, startIndex, count, CompareOptions.IgnoreCase);
		case StringComparison.InvariantCulture:
			return CultureInfo.InvariantCulture.CompareInfo.IndexOf(this, value, startIndex, count, CompareOptions.None);
		case StringComparison.InvariantCultureIgnoreCase:
			return CultureInfo.InvariantCulture.CompareInfo.IndexOf(this, value, startIndex, count, CompareOptions.IgnoreCase);
		case StringComparison.Ordinal:
			return CultureInfo.InvariantCulture.CompareInfo.IndexOf(this, value, startIndex, count, CompareOptions.Ordinal);
		case StringComparison.OrdinalIgnoreCase:
			if (value.IsAscii() && IsAscii())
			{
				return CultureInfo.InvariantCulture.CompareInfo.IndexOf(this, value, startIndex, count, CompareOptions.IgnoreCase);
			}
			return TextInfo.IndexOfStringOrdinalIgnoreCase(this, value, startIndex, count);
		default:
			throw new ArgumentException(Environment.GetResourceString("The string comparison type passed in is currently not supported."), "comparisonType");
		}
	}

	public int LastIndexOf(char value)
	{
		return LastIndexOf(value, Length - 1, Length);
	}

	public int LastIndexOf(char value, int startIndex)
	{
		return LastIndexOf(value, startIndex, startIndex + 1);
	}

	public int LastIndexOfAny(char[] anyOf)
	{
		return LastIndexOfAny(anyOf, Length - 1, Length);
	}

	public int LastIndexOfAny(char[] anyOf, int startIndex)
	{
		return LastIndexOfAny(anyOf, startIndex, startIndex + 1);
	}

	public int LastIndexOf(string value)
	{
		return LastIndexOf(value, Length - 1, Length, StringComparison.CurrentCulture);
	}

	public int LastIndexOf(string value, int startIndex)
	{
		return LastIndexOf(value, startIndex, startIndex + 1, StringComparison.CurrentCulture);
	}

	public int LastIndexOf(string value, int startIndex, int count)
	{
		if (count < 0)
		{
			throw new ArgumentOutOfRangeException("count", Environment.GetResourceString("Count must be positive and count must refer to a location within the string/array/collection."));
		}
		return LastIndexOf(value, startIndex, count, StringComparison.CurrentCulture);
	}

	public int LastIndexOf(string value, StringComparison comparisonType)
	{
		return LastIndexOf(value, Length - 1, Length, comparisonType);
	}

	public int LastIndexOf(string value, int startIndex, StringComparison comparisonType)
	{
		return LastIndexOf(value, startIndex, startIndex + 1, comparisonType);
	}

	[SecuritySafeCritical]
	public int LastIndexOf(string value, int startIndex, int count, StringComparison comparisonType)
	{
		if ((object)value == null)
		{
			throw new ArgumentNullException("value");
		}
		if (Length == 0 && (startIndex == -1 || startIndex == 0))
		{
			if (value.Length != 0)
			{
				return -1;
			}
			return 0;
		}
		if (startIndex < 0 || startIndex > Length)
		{
			throw new ArgumentOutOfRangeException("startIndex", Environment.GetResourceString("Index was out of range. Must be non-negative and less than the size of the collection."));
		}
		if (startIndex == Length)
		{
			startIndex--;
			if (count > 0)
			{
				count--;
			}
			if (value.Length == 0 && count >= 0 && startIndex - count + 1 >= 0)
			{
				return startIndex;
			}
		}
		if (count < 0 || startIndex - count + 1 < 0)
		{
			throw new ArgumentOutOfRangeException("count", Environment.GetResourceString("Count must be positive and count must refer to a location within the string/array/collection."));
		}
		switch (comparisonType)
		{
		case StringComparison.CurrentCulture:
			return CultureInfo.CurrentCulture.CompareInfo.LastIndexOf(this, value, startIndex, count, CompareOptions.None);
		case StringComparison.CurrentCultureIgnoreCase:
			return CultureInfo.CurrentCulture.CompareInfo.LastIndexOf(this, value, startIndex, count, CompareOptions.IgnoreCase);
		case StringComparison.InvariantCulture:
			return CultureInfo.InvariantCulture.CompareInfo.LastIndexOf(this, value, startIndex, count, CompareOptions.None);
		case StringComparison.InvariantCultureIgnoreCase:
			return CultureInfo.InvariantCulture.CompareInfo.LastIndexOf(this, value, startIndex, count, CompareOptions.IgnoreCase);
		case StringComparison.Ordinal:
			return CultureInfo.InvariantCulture.CompareInfo.LastIndexOf(this, value, startIndex, count, CompareOptions.Ordinal);
		case StringComparison.OrdinalIgnoreCase:
			if (value.IsAscii() && IsAscii())
			{
				return CultureInfo.InvariantCulture.CompareInfo.LastIndexOf(this, value, startIndex, count, CompareOptions.IgnoreCase);
			}
			return TextInfo.LastIndexOfStringOrdinalIgnoreCase(this, value, startIndex, count);
		default:
			throw new ArgumentException(Environment.GetResourceString("The string comparison type passed in is currently not supported."), "comparisonType");
		}
	}

	public string PadLeft(int totalWidth)
	{
		return PadHelper(totalWidth, ' ', isRightPadded: false);
	}

	public string PadLeft(int totalWidth, char paddingChar)
	{
		return PadHelper(totalWidth, paddingChar, isRightPadded: false);
	}

	public string PadRight(int totalWidth)
	{
		return PadHelper(totalWidth, ' ', isRightPadded: true);
	}

	public string PadRight(int totalWidth, char paddingChar)
	{
		return PadHelper(totalWidth, paddingChar, isRightPadded: true);
	}

	public bool StartsWith(string value)
	{
		if ((object)value == null)
		{
			throw new ArgumentNullException("value");
		}
		return StartsWith(value, StringComparison.CurrentCulture);
	}

	[ComVisible(false)]
	[SecuritySafeCritical]
	public bool StartsWith(string value, StringComparison comparisonType)
	{
		if ((object)value == null)
		{
			throw new ArgumentNullException("value");
		}
		if (comparisonType < StringComparison.CurrentCulture || comparisonType > StringComparison.OrdinalIgnoreCase)
		{
			throw new ArgumentException(Environment.GetResourceString("The string comparison type passed in is currently not supported."), "comparisonType");
		}
		if ((object)this == value)
		{
			return true;
		}
		if (value.Length == 0)
		{
			return true;
		}
		switch (comparisonType)
		{
		case StringComparison.CurrentCulture:
			return CultureInfo.CurrentCulture.CompareInfo.IsPrefix(this, value, CompareOptions.None);
		case StringComparison.CurrentCultureIgnoreCase:
			return CultureInfo.CurrentCulture.CompareInfo.IsPrefix(this, value, CompareOptions.IgnoreCase);
		case StringComparison.InvariantCulture:
			return CultureInfo.InvariantCulture.CompareInfo.IsPrefix(this, value, CompareOptions.None);
		case StringComparison.InvariantCultureIgnoreCase:
			return CultureInfo.InvariantCulture.CompareInfo.IsPrefix(this, value, CompareOptions.IgnoreCase);
		case StringComparison.Ordinal:
			if (Length < value.Length)
			{
				return false;
			}
			return nativeCompareOrdinalEx(this, 0, value, 0, value.Length) == 0;
		case StringComparison.OrdinalIgnoreCase:
			if (Length < value.Length)
			{
				return false;
			}
			return TextInfo.CompareOrdinalIgnoreCaseEx(this, 0, value, 0, value.Length, value.Length) == 0;
		default:
			throw new ArgumentException(Environment.GetResourceString("The string comparison type passed in is currently not supported."), "comparisonType");
		}
	}

	public bool StartsWith(string value, bool ignoreCase, CultureInfo culture)
	{
		if ((object)value == null)
		{
			throw new ArgumentNullException("value");
		}
		if ((object)this == value)
		{
			return true;
		}
		CultureInfo cultureInfo = ((culture != null) ? culture : CultureInfo.CurrentCulture);
		return cultureInfo.CompareInfo.IsPrefix(this, value, ignoreCase ? CompareOptions.IgnoreCase : CompareOptions.None);
	}

	public string ToLower()
	{
		return ToLower(CultureInfo.CurrentCulture);
	}

	public string ToLower(CultureInfo culture)
	{
		if (culture == null)
		{
			throw new ArgumentNullException("culture");
		}
		return culture.TextInfo.ToLower(this);
	}

	public string ToLowerInvariant()
	{
		return ToLower(CultureInfo.InvariantCulture);
	}

	public string ToUpper()
	{
		return ToUpper(CultureInfo.CurrentCulture);
	}

	public string ToUpper(CultureInfo culture)
	{
		if (culture == null)
		{
			throw new ArgumentNullException("culture");
		}
		return culture.TextInfo.ToUpper(this);
	}

	public string ToUpperInvariant()
	{
		return ToUpper(CultureInfo.InvariantCulture);
	}

	public override string ToString()
	{
		return this;
	}

	public string ToString(IFormatProvider provider)
	{
		return this;
	}

	public object Clone()
	{
		return this;
	}

	private static bool IsBOMWhitespace(char c)
	{
		return false;
	}

	public string Trim()
	{
		return TrimHelper(2);
	}

	[SecuritySafeCritical]
	private string TrimHelper(int trimType)
	{
		int num = Length - 1;
		int i = 0;
		if (trimType != 1)
		{
			for (i = 0; i < Length && (char.IsWhiteSpace(this[i]) || IsBOMWhitespace(this[i])); i++)
			{
			}
		}
		if (trimType != 0)
		{
			num = Length - 1;
			while (num >= i && (char.IsWhiteSpace(this[num]) || IsBOMWhitespace(this[i])))
			{
				num--;
			}
		}
		return CreateTrimmedString(i, num);
	}

	[SecuritySafeCritical]
	private string TrimHelper(char[] trimChars, int trimType)
	{
		int num = Length - 1;
		int i = 0;
		if (trimType != 1)
		{
			for (i = 0; i < Length; i++)
			{
				int num2 = 0;
				char c = this[i];
				for (num2 = 0; num2 < trimChars.Length && trimChars[num2] != c; num2++)
				{
				}
				if (num2 == trimChars.Length)
				{
					break;
				}
			}
		}
		if (trimType != 0)
		{
			for (num = Length - 1; num >= i; num--)
			{
				int num3 = 0;
				char c2 = this[num];
				for (num3 = 0; num3 < trimChars.Length && trimChars[num3] != c2; num3++)
				{
				}
				if (num3 == trimChars.Length)
				{
					break;
				}
			}
		}
		return CreateTrimmedString(i, num);
	}

	[SecurityCritical]
	private string CreateTrimmedString(int start, int end)
	{
		int num = end - start + 1;
		if (num == Length)
		{
			return this;
		}
		if (num == 0)
		{
			return Empty;
		}
		return InternalSubString(start, num);
	}

	[SecuritySafeCritical]
	public unsafe string Insert(int startIndex, string value)
	{
		if ((object)value == null)
		{
			throw new ArgumentNullException("value");
		}
		if (startIndex < 0 || startIndex > Length)
		{
			throw new ArgumentOutOfRangeException("startIndex");
		}
		int length = Length;
		int length2 = value.Length;
		int num = length + length2;
		if (num == 0)
		{
			return Empty;
		}
		string text = FastAllocateString(num);
		fixed (char* ptr2 = &m_firstChar)
		{
			fixed (char* smem = &value.m_firstChar)
			{
				fixed (char* ptr = &text.m_firstChar)
				{
					wstrcpy(ptr, ptr2, startIndex);
					wstrcpy(ptr + startIndex, smem, length2);
					wstrcpy(ptr + startIndex + length2, ptr2 + startIndex, length - startIndex);
				}
			}
		}
		return text;
	}

	public string Replace(char oldChar, char newChar)
	{
		return ReplaceInternal(oldChar, newChar);
	}

	public string Replace(string oldValue, string newValue)
	{
		if ((object)oldValue == null)
		{
			throw new ArgumentNullException("oldValue");
		}
		return ReplaceInternal(oldValue, newValue);
	}

	[SecuritySafeCritical]
	public unsafe string Remove(int startIndex, int count)
	{
		if (startIndex < 0)
		{
			throw new ArgumentOutOfRangeException("startIndex", Environment.GetResourceString("StartIndex cannot be less than zero."));
		}
		if (count < 0)
		{
			throw new ArgumentOutOfRangeException("count", Environment.GetResourceString("Count cannot be less than zero."));
		}
		if (count > Length - startIndex)
		{
			throw new ArgumentOutOfRangeException("count", Environment.GetResourceString("Index and count must refer to a location within the string."));
		}
		int num = Length - count;
		if (num == 0)
		{
			return Empty;
		}
		string text = FastAllocateString(num);
		fixed (char* ptr2 = &m_firstChar)
		{
			fixed (char* ptr = &text.m_firstChar)
			{
				wstrcpy(ptr, ptr2, startIndex);
				wstrcpy(ptr + startIndex, ptr2 + startIndex + count, num - startIndex);
			}
		}
		return text;
	}

	public string Remove(int startIndex)
	{
		if (startIndex < 0)
		{
			throw new ArgumentOutOfRangeException("startIndex", Environment.GetResourceString("StartIndex cannot be less than zero."));
		}
		if (startIndex >= Length)
		{
			throw new ArgumentOutOfRangeException("startIndex", Environment.GetResourceString("startIndex must be less than length of string."));
		}
		return Substring(0, startIndex);
	}

	public static string Format(string format, object arg0)
	{
		return FormatHelper(null, format, new ParamsArray(arg0));
	}

	public static string Format(string format, object arg0, object arg1)
	{
		return FormatHelper(null, format, new ParamsArray(arg0, arg1));
	}

	public static string Format(string format, object arg0, object arg1, object arg2)
	{
		return FormatHelper(null, format, new ParamsArray(arg0, arg1, arg2));
	}

	public static string Format(string format, params object[] args)
	{
		if (args == null)
		{
			throw new ArgumentNullException(((object)format == null) ? "format" : "args");
		}
		return FormatHelper(null, format, new ParamsArray(args));
	}

	public static string Format(IFormatProvider provider, string format, object arg0)
	{
		return FormatHelper(provider, format, new ParamsArray(arg0));
	}

	public static string Format(IFormatProvider provider, string format, object arg0, object arg1)
	{
		return FormatHelper(provider, format, new ParamsArray(arg0, arg1));
	}

	public static string Format(IFormatProvider provider, string format, object arg0, object arg1, object arg2)
	{
		return FormatHelper(provider, format, new ParamsArray(arg0, arg1, arg2));
	}

	public static string Format(IFormatProvider provider, string format, params object[] args)
	{
		if (args == null)
		{
			throw new ArgumentNullException(((object)format == null) ? "format" : "args");
		}
		return FormatHelper(provider, format, new ParamsArray(args));
	}

	private static string FormatHelper(IFormatProvider provider, string format, ParamsArray args)
	{
		if ((object)format == null)
		{
			throw new ArgumentNullException("format");
		}
		return StringBuilderCache.GetStringAndRelease(StringBuilderCache.Acquire(format.Length + args.Length * 8).AppendFormatHelper(provider, format, args));
	}

	[SecuritySafeCritical]
	public unsafe static string Copy(string str)
	{
		if ((object)str == null)
		{
			throw new ArgumentNullException("str");
		}
		int length = str.Length;
		string text = FastAllocateString(length);
		fixed (char* dmem = &text.m_firstChar)
		{
			fixed (char* smem = &str.m_firstChar)
			{
				wstrcpy(dmem, smem, length);
			}
		}
		return text;
	}

	public static string Concat(object arg0)
	{
		if (arg0 == null)
		{
			return Empty;
		}
		return arg0.ToString();
	}

	public static string Concat(object arg0, object arg1)
	{
		if (arg0 == null)
		{
			arg0 = Empty;
		}
		if (arg1 == null)
		{
			arg1 = Empty;
		}
		return arg0.ToString() + arg1.ToString();
	}

	public static string Concat(object arg0, object arg1, object arg2)
	{
		if (arg0 == null)
		{
			arg0 = Empty;
		}
		if (arg1 == null)
		{
			arg1 = Empty;
		}
		if (arg2 == null)
		{
			arg2 = Empty;
		}
		return arg0.ToString() + arg1.ToString() + arg2.ToString();
	}

	[CLSCompliant(false)]
	public static string Concat(object arg0, object arg1, object arg2, object arg3, __arglist)
	{
		ArgIterator argIterator = new ArgIterator(__arglist);
		int num = argIterator.GetRemainingCount() + 4;
		object[] array = new object[num];
		array[0] = arg0;
		array[1] = arg1;
		array[2] = arg2;
		array[3] = arg3;
		for (int i = 4; i < num; i++)
		{
			array[i] = TypedReference.ToObject(argIterator.GetNextArg());
		}
		return Concat(array);
	}

	public static string Concat(params object[] args)
	{
		if (args == null)
		{
			throw new ArgumentNullException("args");
		}
		string[] array = new string[args.Length];
		int num = 0;
		for (int i = 0; i < args.Length; i++)
		{
			object obj = args[i];
			array[i] = ((obj == null) ? Empty : obj.ToString());
			if ((object)array[i] == null)
			{
				array[i] = Empty;
			}
			num += array[i].Length;
			if (num < 0)
			{
				throw new OutOfMemoryException();
			}
		}
		return ConcatArray(array, num);
	}

	[ComVisible(false)]
	public static string Concat<T>(IEnumerable<T> values)
	{
		if (values == null)
		{
			throw new ArgumentNullException("values");
		}
		StringBuilder stringBuilder = StringBuilderCache.Acquire();
		using (IEnumerator<T> enumerator = values.GetEnumerator())
		{
			while (enumerator.MoveNext())
			{
				if (enumerator.Current != null)
				{
					string text = enumerator.Current.ToString();
					if ((object)text != null)
					{
						stringBuilder.Append(text);
					}
				}
			}
		}
		return StringBuilderCache.GetStringAndRelease(stringBuilder);
	}

	[ComVisible(false)]
	public static string Concat(IEnumerable<string> values)
	{
		if (values == null)
		{
			throw new ArgumentNullException("values");
		}
		StringBuilder stringBuilder = StringBuilderCache.Acquire();
		using (IEnumerator<string> enumerator = values.GetEnumerator())
		{
			while (enumerator.MoveNext())
			{
				if ((object)enumerator.Current != null)
				{
					stringBuilder.Append(enumerator.Current);
				}
			}
		}
		return StringBuilderCache.GetStringAndRelease(stringBuilder);
	}

	[SecuritySafeCritical]
	public static string Concat(string str0, string str1)
	{
		if (IsNullOrEmpty(str0))
		{
			if (IsNullOrEmpty(str1))
			{
				return Empty;
			}
			return str1;
		}
		if (IsNullOrEmpty(str1))
		{
			return str0;
		}
		int length = str0.Length;
		string text = FastAllocateString(length + str1.Length);
		FillStringChecked(text, 0, str0);
		FillStringChecked(text, length, str1);
		return text;
	}

	[SecuritySafeCritical]
	public static string Concat(string str0, string str1, string str2)
	{
		if ((object)str0 == null && (object)str1 == null && (object)str2 == null)
		{
			return Empty;
		}
		if ((object)str0 == null)
		{
			str0 = Empty;
		}
		if ((object)str1 == null)
		{
			str1 = Empty;
		}
		if ((object)str2 == null)
		{
			str2 = Empty;
		}
		string text = FastAllocateString(str0.Length + str1.Length + str2.Length);
		FillStringChecked(text, 0, str0);
		FillStringChecked(text, str0.Length, str1);
		FillStringChecked(text, str0.Length + str1.Length, str2);
		return text;
	}

	[SecuritySafeCritical]
	public static string Concat(string str0, string str1, string str2, string str3)
	{
		if ((object)str0 == null && (object)str1 == null && (object)str2 == null && (object)str3 == null)
		{
			return Empty;
		}
		if ((object)str0 == null)
		{
			str0 = Empty;
		}
		if ((object)str1 == null)
		{
			str1 = Empty;
		}
		if ((object)str2 == null)
		{
			str2 = Empty;
		}
		if ((object)str3 == null)
		{
			str3 = Empty;
		}
		string text = FastAllocateString(str0.Length + str1.Length + str2.Length + str3.Length);
		FillStringChecked(text, 0, str0);
		FillStringChecked(text, str0.Length, str1);
		FillStringChecked(text, str0.Length + str1.Length, str2);
		FillStringChecked(text, str0.Length + str1.Length + str2.Length, str3);
		return text;
	}

	[SecuritySafeCritical]
	private static string ConcatArray(string[] values, int totalLength)
	{
		string text = FastAllocateString(totalLength);
		int num = 0;
		for (int i = 0; i < values.Length; i++)
		{
			FillStringChecked(text, num, values[i]);
			num += values[i].Length;
		}
		return text;
	}

	public static string Concat(params string[] values)
	{
		if (values == null)
		{
			throw new ArgumentNullException("values");
		}
		int num = 0;
		string[] array = new string[values.Length];
		for (int i = 0; i < values.Length; i++)
		{
			string text = values[i];
			array[i] = (((object)text == null) ? Empty : text);
			num += array[i].Length;
			if (num < 0)
			{
				throw new OutOfMemoryException();
			}
		}
		return ConcatArray(array, num);
	}

	[SecuritySafeCritical]
	public static string Intern(string str)
	{
		if ((object)str == null)
		{
			throw new ArgumentNullException("str");
		}
		return InternalIntern(str);
	}

	[SecuritySafeCritical]
	public static string IsInterned(string str)
	{
		if ((object)str == null)
		{
			throw new ArgumentNullException("str");
		}
		return InternalIsInterned(str);
	}

	public TypeCode GetTypeCode()
	{
		return TypeCode.String;
	}

	bool IConvertible.ToBoolean(IFormatProvider provider)
	{
		return Convert.ToBoolean(this, provider);
	}

	char IConvertible.ToChar(IFormatProvider provider)
	{
		return Convert.ToChar(this, provider);
	}

	sbyte IConvertible.ToSByte(IFormatProvider provider)
	{
		return Convert.ToSByte(this, provider);
	}

	byte IConvertible.ToByte(IFormatProvider provider)
	{
		return Convert.ToByte(this, provider);
	}

	short IConvertible.ToInt16(IFormatProvider provider)
	{
		return Convert.ToInt16(this, provider);
	}

	ushort IConvertible.ToUInt16(IFormatProvider provider)
	{
		return Convert.ToUInt16(this, provider);
	}

	int IConvertible.ToInt32(IFormatProvider provider)
	{
		return Convert.ToInt32(this, provider);
	}

	uint IConvertible.ToUInt32(IFormatProvider provider)
	{
		return Convert.ToUInt32(this, provider);
	}

	long IConvertible.ToInt64(IFormatProvider provider)
	{
		return Convert.ToInt64(this, provider);
	}

	ulong IConvertible.ToUInt64(IFormatProvider provider)
	{
		return Convert.ToUInt64(this, provider);
	}

	float IConvertible.ToSingle(IFormatProvider provider)
	{
		return Convert.ToSingle(this, provider);
	}

	double IConvertible.ToDouble(IFormatProvider provider)
	{
		return Convert.ToDouble(this, provider);
	}

	decimal IConvertible.ToDecimal(IFormatProvider provider)
	{
		return Convert.ToDecimal(this, provider);
	}

	DateTime IConvertible.ToDateTime(IFormatProvider provider)
	{
		return Convert.ToDateTime(this, provider);
	}

	object IConvertible.ToType(Type type, IFormatProvider provider)
	{
		return Convert.DefaultToType(this, type, provider);
	}

	public CharEnumerator GetEnumerator()
	{
		return new CharEnumerator(this);
	}

	IEnumerator<char> IEnumerable<char>.GetEnumerator()
	{
		return new CharEnumerator(this);
	}

	IEnumerator IEnumerable.GetEnumerator()
	{
		return new CharEnumerator(this);
	}

	[SecurityCritical]
	internal unsafe static void InternalCopy(string src, IntPtr dest, int len)
	{
		if (len != 0)
		{
			fixed (char* ptr = &src.m_firstChar)
			{
				byte* src2 = (byte*)ptr;
				byte* dest2 = (byte*)(void*)dest;
				Buffer.Memcpy(dest2, src2, len);
			}
		}
	}

	internal unsafe static int CompareOrdinalUnchecked(string strA, int indexA, int lenA, string strB, int indexB, int lenB)
	{
		if ((object)strA == null)
		{
			if ((object)strB != null)
			{
				return -1;
			}
			return 0;
		}
		if ((object)strB == null)
		{
			return 1;
		}
		int num = Math.Min(lenA, strA.m_stringLength - indexA);
		int num2 = Math.Min(lenB, strB.m_stringLength - indexB);
		if (num == num2 && indexA == indexB && (object)strA == strB)
		{
			return 0;
		}
		fixed (char* ptr = strA)
		{
			fixed (char* ptr4 = strB)
			{
				char* ptr2 = ptr + indexA;
				char* ptr3 = ptr2 + Math.Min(num, num2);
				char* ptr5 = ptr4 + indexB;
				while (ptr2 < ptr3)
				{
					if (*ptr2 != *ptr5)
					{
						return *ptr2 - *ptr5;
					}
					ptr2++;
					ptr5++;
				}
				return num - num2;
			}
		}
	}

	public int IndexOf(char value, int startIndex, int count)
	{
		if (startIndex < 0 || startIndex > m_stringLength)
		{
			throw new ArgumentOutOfRangeException("startIndex", "Cannot be negative and must be< 0");
		}
		if (count < 0)
		{
			throw new ArgumentOutOfRangeException("count", "< 0");
		}
		if (startIndex > m_stringLength - count)
		{
			throw new ArgumentOutOfRangeException("count", "startIndex + count > this.m_stringLength");
		}
		if ((startIndex == 0 && m_stringLength == 0) || startIndex == m_stringLength || count == 0)
		{
			return -1;
		}
		return IndexOfUnchecked(value, startIndex, count);
	}

	internal unsafe int IndexOfUnchecked(char value, int startIndex, int count)
	{
		fixed (char* ptr = &m_firstChar)
		{
			char* ptr2 = ptr + startIndex;
			char* ptr3;
			for (ptr3 = ptr2 + (count >> 3 << 3); ptr2 != ptr3; ptr2 += 8)
			{
				if (*ptr2 == value)
				{
					return (int)(ptr2 - ptr);
				}
				if (ptr2[1] == value)
				{
					return (int)(ptr2 - ptr + 1);
				}
				if (ptr2[2] == value)
				{
					return (int)(ptr2 - ptr + 2);
				}
				if (ptr2[3] == value)
				{
					return (int)(ptr2 - ptr + 3);
				}
				if (ptr2[4] == value)
				{
					return (int)(ptr2 - ptr + 4);
				}
				if (ptr2[5] == value)
				{
					return (int)(ptr2 - ptr + 5);
				}
				if (ptr2[6] == value)
				{
					return (int)(ptr2 - ptr + 6);
				}
				if (ptr2[7] == value)
				{
					return (int)(ptr2 - ptr + 7);
				}
			}
			for (ptr3 += count & 7; ptr2 != ptr3; ptr2++)
			{
				if (*ptr2 == value)
				{
					return (int)(ptr2 - ptr);
				}
			}
			return -1;
		}
	}

	internal unsafe int IndexOfUnchecked(string value, int startIndex, int count)
	{
		int length = value.Length;
		if (count < length)
		{
			return -1;
		}
		if (length <= 1)
		{
			if (length == 1)
			{
				return IndexOfUnchecked(value[0], startIndex, count);
			}
			return startIndex;
		}
		fixed (char* ptr = &m_firstChar)
		{
			fixed (char* ptr4 = value)
			{
				char* ptr2 = ptr + startIndex;
				for (char* ptr3 = ptr2 + count - length + 1; ptr2 != ptr3; ptr2++)
				{
					if (*ptr2 != *ptr4)
					{
						continue;
					}
					int num = 1;
					while (true)
					{
						if (num < length)
						{
							if (ptr2[num] != ptr4[num])
							{
								break;
							}
							num++;
							continue;
						}
						return (int)(ptr2 - ptr);
					}
				}
			}
		}
		return -1;
	}

	public int IndexOfAny(char[] anyOf, int startIndex, int count)
	{
		if (anyOf == null)
		{
			throw new ArgumentNullException();
		}
		if (startIndex < 0 || startIndex > m_stringLength)
		{
			throw new ArgumentOutOfRangeException();
		}
		if (count < 0 || startIndex > m_stringLength - count)
		{
			throw new ArgumentOutOfRangeException("count", "Count cannot be negative, and startIndex + count must be less than m_stringLength of the string.");
		}
		return IndexOfAnyUnchecked(anyOf, startIndex, count);
	}

	private unsafe int IndexOfAnyUnchecked(char[] anyOf, int startIndex, int count)
	{
		if (anyOf.Length == 0)
		{
			return -1;
		}
		if (anyOf.Length == 1)
		{
			return IndexOfUnchecked(anyOf[0], startIndex, count);
		}
		fixed (char* ptr = anyOf)
		{
			int num = *ptr;
			int num2 = *ptr;
			char* ptr2 = ptr + anyOf.Length;
			char* ptr3 = ptr;
			while (++ptr3 != ptr2)
			{
				if (*ptr3 > num)
				{
					num = *ptr3;
				}
				else if (*ptr3 < num2)
				{
					num2 = *ptr3;
				}
			}
			fixed (char* ptr4 = &m_firstChar)
			{
				char* ptr5 = ptr4 + startIndex;
				char* ptr6 = ptr5 + count;
				while (ptr5 != ptr6)
				{
					if (*ptr5 > num || *ptr5 < num2)
					{
						ptr5++;
						continue;
					}
					if (*ptr5 == *ptr)
					{
						return (int)(ptr5 - ptr4);
					}
					ptr3 = ptr;
					while (++ptr3 != ptr2)
					{
						if (*ptr5 == *ptr3)
						{
							return (int)(ptr5 - ptr4);
						}
					}
					ptr5++;
				}
			}
		}
		return -1;
	}

	public int LastIndexOf(char value, int startIndex, int count)
	{
		if (m_stringLength == 0)
		{
			return -1;
		}
		if (startIndex < 0 || startIndex >= Length)
		{
			throw new ArgumentOutOfRangeException("startIndex", "< 0 || >= this.Length");
		}
		if (count < 0 || count > Length)
		{
			throw new ArgumentOutOfRangeException("count", "< 0 || > this.Length");
		}
		if (startIndex - count + 1 < 0)
		{
			throw new ArgumentOutOfRangeException("startIndex - count + 1 < 0");
		}
		return LastIndexOfUnchecked(value, startIndex, count);
	}

	internal unsafe int LastIndexOfUnchecked(char value, int startIndex, int count)
	{
		fixed (char* ptr = &m_firstChar)
		{
			char* ptr2 = ptr + startIndex;
			char* ptr3 = ptr2 - (count >> 3 << 3);
			while (ptr2 != ptr3)
			{
				if (*ptr2 == value)
				{
					return (int)(ptr2 - ptr);
				}
				if (ptr2[-1] == value)
				{
					return (int)(ptr2 - ptr) - 1;
				}
				if (ptr2[-2] == value)
				{
					return (int)(ptr2 - ptr) - 2;
				}
				if (ptr2[-3] == value)
				{
					return (int)(ptr2 - ptr) - 3;
				}
				if (ptr2[-4] == value)
				{
					return (int)(ptr2 - ptr) - 4;
				}
				if (ptr2[-5] == value)
				{
					return (int)(ptr2 - ptr) - 5;
				}
				if (ptr2[-6] == value)
				{
					return (int)(ptr2 - ptr) - 6;
				}
				if (ptr2[-7] == value)
				{
					return (int)(ptr2 - ptr) - 7;
				}
				ptr2 -= 8;
			}
			ptr3 -= count & 7;
			while (ptr2 != ptr3)
			{
				if (*ptr2 == value)
				{
					return (int)(ptr2 - ptr);
				}
				ptr2--;
			}
			return -1;
		}
	}

	public int LastIndexOfAny(char[] anyOf, int startIndex, int count)
	{
		if (anyOf == null)
		{
			throw new ArgumentNullException();
		}
		if (m_stringLength == 0)
		{
			return -1;
		}
		if (startIndex < 0 || startIndex >= Length)
		{
			throw new ArgumentOutOfRangeException("startIndex", "< 0 || > this.Length");
		}
		if (count < 0 || count > Length)
		{
			throw new ArgumentOutOfRangeException("count", "< 0 || > this.Length");
		}
		if (startIndex - count + 1 < 0)
		{
			throw new ArgumentOutOfRangeException("startIndex - count + 1 < 0");
		}
		if (m_stringLength == 0)
		{
			return -1;
		}
		return LastIndexOfAnyUnchecked(anyOf, startIndex, count);
	}

	private unsafe int LastIndexOfAnyUnchecked(char[] anyOf, int startIndex, int count)
	{
		if (anyOf.Length == 1)
		{
			return LastIndexOfUnchecked(anyOf[0], startIndex, count);
		}
		fixed (char* ptr = &m_firstChar)
		{
			fixed (char* ptr4 = anyOf)
			{
				char* ptr2 = ptr + startIndex;
				char* ptr3 = ptr2 - count;
				char* ptr5 = ptr4 + anyOf.Length;
				while (ptr2 != ptr3)
				{
					for (char* ptr6 = ptr4; ptr6 != ptr5; ptr6++)
					{
						if (*ptr6 == *ptr2)
						{
							return (int)(ptr2 - ptr);
						}
					}
					ptr2--;
				}
				return -1;
			}
		}
	}

	internal static int nativeCompareOrdinalEx(string strA, int indexA, string strB, int indexB, int count)
	{
		if (count < 0)
		{
			throw new ArgumentOutOfRangeException("count", Environment.GetResourceString("Count cannot be less than zero."));
		}
		if (indexA < 0 || indexA > strA.Length)
		{
			throw new ArgumentOutOfRangeException("indexA", Environment.GetResourceString("Index was out of range. Must be non-negative and less than the size of the collection."));
		}
		if (indexB < 0 || indexB > strB.Length)
		{
			throw new ArgumentOutOfRangeException("indexB", Environment.GetResourceString("Index was out of range. Must be non-negative and less than the size of the collection."));
		}
		return CompareOrdinalUnchecked(strA, indexA, count, strB, indexB, count);
	}

	private unsafe string ReplaceInternal(char oldChar, char newChar)
	{
		if (m_stringLength == 0 || oldChar == newChar)
		{
			return this;
		}
		int num = IndexOfUnchecked(oldChar, 0, m_stringLength);
		if (num == -1)
		{
			return this;
		}
		if (num < 4)
		{
			num = 0;
		}
		string text = FastAllocateString(m_stringLength);
		fixed (char* ptr = text)
		{
			fixed (char* ptr2 = &m_firstChar)
			{
				if (num != 0)
				{
					CharCopy(ptr, ptr2, num);
				}
				char* ptr3 = ptr + m_stringLength;
				char* ptr4 = ptr + num;
				char* ptr5 = ptr2 + num;
				for (; ptr4 != ptr3; ptr4++)
				{
					if (*ptr5 == oldChar)
					{
						*ptr4 = newChar;
					}
					else
					{
						*ptr4 = *ptr5;
					}
					ptr5++;
				}
			}
		}
		return text;
	}

	internal string ReplaceInternal(string oldValue, string newValue)
	{
		if ((object)oldValue == null)
		{
			throw new ArgumentNullException("oldValue");
		}
		if (oldValue.Length == 0)
		{
			throw new ArgumentException("oldValue is the empty string.");
		}
		if (Length == 0)
		{
			return this;
		}
		if ((object)newValue == null)
		{
			newValue = Empty;
		}
		return ReplaceUnchecked(oldValue, newValue);
	}

	private unsafe string ReplaceUnchecked(string oldValue, string newValue)
	{
		if (oldValue.m_stringLength > m_stringLength)
		{
			return this;
		}
		if (oldValue.m_stringLength == 1 && newValue.m_stringLength == 1)
		{
			return Replace(oldValue[0], newValue[0]);
		}
		int* ptr = stackalloc int[200];
		fixed (char* ptr3 = &m_firstChar)
		{
			fixed (char* src = newValue)
			{
				int num = 0;
				int num2 = 0;
				while (num < m_stringLength)
				{
					int num3 = IndexOfUnchecked(oldValue, num, m_stringLength - num);
					if (num3 < 0)
					{
						break;
					}
					if (num2 < 200)
					{
						ptr[num2++] = num3;
						num = num3 + oldValue.m_stringLength;
						continue;
					}
					return ReplaceFallback(oldValue, newValue, 200);
				}
				if (num2 == 0)
				{
					return this;
				}
				int num4 = 0;
				try
				{
					num4 = checked(m_stringLength + (newValue.m_stringLength - oldValue.m_stringLength) * num2);
				}
				catch (OverflowException)
				{
					throw new OutOfMemoryException();
				}
				string text = FastAllocateString(num4);
				int num5 = 0;
				int num6 = 0;
				fixed (char* ptr2 = text)
				{
					for (int i = 0; i < num2; i++)
					{
						int num7 = ptr[i] - num6;
						CharCopy(ptr2 + num5, ptr3 + num6, num7);
						num5 += num7;
						num6 = ptr[i] + oldValue.m_stringLength;
						CharCopy(ptr2 + num5, src, newValue.m_stringLength);
						num5 += newValue.m_stringLength;
					}
					CharCopy(ptr2 + num5, ptr3 + num6, m_stringLength - num6);
				}
				return text;
			}
		}
	}

	private string ReplaceFallback(string oldValue, string newValue, int testedCount)
	{
		StringBuilder stringBuilder = new StringBuilder(m_stringLength + (newValue.m_stringLength - oldValue.m_stringLength) * testedCount);
		int num = 0;
		while (num < m_stringLength)
		{
			int num2 = IndexOfUnchecked(oldValue, num, m_stringLength - num);
			if (num2 < 0)
			{
				stringBuilder.Append(InternalSubString(num, m_stringLength - num));
				break;
			}
			stringBuilder.Append(InternalSubString(num, num2 - num));
			stringBuilder.Append(newValue);
			num = num2 + oldValue.m_stringLength;
		}
		return stringBuilder.ToString();
	}

	private unsafe string PadHelper(int totalWidth, char paddingChar, bool isRightPadded)
	{
		if (totalWidth < 0)
		{
			throw new ArgumentOutOfRangeException("totalWidth", "Non-negative number required");
		}
		if (totalWidth <= m_stringLength)
		{
			return this;
		}
		string text = FastAllocateString(totalWidth);
		fixed (char* ptr = text)
		{
			fixed (char* src = &m_firstChar)
			{
				if (isRightPadded)
				{
					CharCopy(ptr, src, m_stringLength);
					char* ptr2 = ptr + totalWidth;
					char* ptr3 = ptr + m_stringLength;
					while (ptr3 < ptr2)
					{
						*(ptr3++) = paddingChar;
					}
				}
				else
				{
					char* ptr4 = ptr;
					char* ptr5 = ptr4 + totalWidth - m_stringLength;
					while (ptr4 < ptr5)
					{
						*(ptr4++) = paddingChar;
					}
					CharCopy(ptr4, src, m_stringLength);
				}
			}
		}
		return text;
	}

	internal bool StartsWithOrdinalUnchecked(string value)
	{
		if (m_stringLength >= value.m_stringLength)
		{
			return CompareOrdinalUnchecked(this, 0, value.m_stringLength, value, 0, value.m_stringLength) == 0;
		}
		return false;
	}

	internal unsafe bool IsAscii()
	{
		fixed (char* ptr = &m_firstChar)
		{
			char* ptr2 = ptr + m_stringLength;
			for (char* ptr3 = ptr; ptr3 != ptr2; ptr3++)
			{
				if (*ptr3 >= '\u0080')
				{
					return false;
				}
			}
		}
		return true;
	}

	internal bool IsFastSort()
	{
		return false;
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern string InternalIsInterned(string str);

	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern string InternalIntern(string str);

	internal unsafe static void CharCopy(char* dest, char* src, int count)
	{
		if (((uint)((int)dest | (int)src) & 3u) != 0)
		{
			if (((uint)(int)dest & 2u) != 0 && ((uint)(int)src & 2u) != 0 && count > 0)
			{
				*dest = *src;
				dest++;
				src++;
				count--;
			}
			if (((uint)((int)dest | (int)src) & 2u) != 0)
			{
				Buffer.memcpy2((byte*)dest, (byte*)src, count * 2);
				return;
			}
		}
		Buffer.memcpy4((byte*)dest, (byte*)src, count * 2);
	}

	private unsafe static void memset(byte* dest, int val, int len)
	{
		if (len < 8)
		{
			while (len != 0)
			{
				*dest = (byte)val;
				dest++;
				len--;
			}
			return;
		}
		if (val != 0)
		{
			val |= val << 8;
			val |= val << 16;
		}
		int num = (int)dest & 3;
		if (num != 0)
		{
			num = 4 - num;
			len -= num;
			do
			{
				*dest = (byte)val;
				dest++;
				num--;
			}
			while (num != 0);
		}
		while (len >= 16)
		{
			*(int*)dest = val;
			*(int*)(dest + 4) = val;
			*(int*)(dest + (nint)2 * (nint)4) = val;
			*(int*)(dest + (nint)3 * (nint)4) = val;
			dest += 16;
			len -= 16;
		}
		while (len >= 4)
		{
			*(int*)dest = val;
			dest += 4;
			len -= 4;
		}
		while (len > 0)
		{
			*dest = (byte)val;
			dest++;
			len--;
		}
	}

	private unsafe static void memcpy(byte* dest, byte* src, int size)
	{
		Buffer.Memcpy(dest, src, size);
	}

	internal unsafe static void bzero(byte* dest, int len)
	{
		memset(dest, 0, len);
	}

	internal unsafe static void bzero_aligned_1(byte* dest, int len)
	{
		*dest = 0;
	}

	internal unsafe static void bzero_aligned_2(byte* dest, int len)
	{
		*(short*)dest = 0;
	}

	internal unsafe static void bzero_aligned_4(byte* dest, int len)
	{
		*(int*)dest = 0;
	}

	internal unsafe static void bzero_aligned_8(byte* dest, int len)
	{
		*(long*)dest = 0L;
	}

	internal unsafe static void memcpy_aligned_1(byte* dest, byte* src, int size)
	{
		*dest = *src;
	}

	internal unsafe static void memcpy_aligned_2(byte* dest, byte* src, int size)
	{
		*(short*)dest = *(short*)src;
	}

	internal unsafe static void memcpy_aligned_4(byte* dest, byte* src, int size)
	{
		*(int*)dest = *(int*)src;
	}

	internal unsafe static void memcpy_aligned_8(byte* dest, byte* src, int size)
	{
		*(long*)dest = *(long*)src;
	}

	private unsafe string CreateString(sbyte* value)
	{
		if (value == null)
		{
			return Empty;
		}
		byte* ptr = (byte*)value;
		int num = 0;
		try
		{
			while (*(ptr++) != 0)
			{
				num++;
			}
		}
		catch (NullReferenceException)
		{
			throw new ArgumentOutOfRangeException("ptr", "Value does not refer to a valid string.");
		}
		return CreateString(value, 0, num, null);
	}

	private unsafe string CreateString(sbyte* value, int startIndex, int length)
	{
		return CreateString(value, startIndex, length, null);
	}

	private unsafe string CreateString(char* value)
	{
		return CtorCharPtr(value);
	}

	private unsafe string CreateString(char* value, int startIndex, int length)
	{
		return CtorCharPtrStartLength(value, startIndex, length);
	}

	private string CreateString(char[] val, int startIndex, int length)
	{
		return CtorCharArrayStartLength(val, startIndex, length);
	}

	private string CreateString(char[] val)
	{
		return CtorCharArray(val);
	}

	private unsafe string CreateString(char c, int count)
	{
		if (count < 0)
		{
			throw new ArgumentOutOfRangeException("count");
		}
		if (count == 0)
		{
			return Empty;
		}
		string text = FastAllocateString(count);
		fixed (char* ptr = text)
		{
			char* ptr2 = ptr;
			for (char* ptr3 = ptr2 + count; ptr2 < ptr3; ptr2++)
			{
				*ptr2 = c;
			}
		}
		return text;
	}

	private unsafe string CreateString(sbyte* value, int startIndex, int length, Encoding enc)
	{
		if (length < 0)
		{
			throw new ArgumentOutOfRangeException("length", "Non-negative number required.");
		}
		if (startIndex < 0)
		{
			throw new ArgumentOutOfRangeException("startIndex", "Non-negative number required.");
		}
		if (value + startIndex < value)
		{
			throw new ArgumentOutOfRangeException("startIndex", "Value, startIndex and length do not refer to a valid string.");
		}
		if (enc == null)
		{
			if (value == null)
			{
				throw new ArgumentNullException("value");
			}
			if (length == 0)
			{
				return Empty;
			}
			enc = Encoding.Default;
		}
		byte[] array = new byte[length];
		if (length != 0)
		{
			fixed (byte* dest = array)
			{
				try
				{
					if (value == null)
					{
						throw new ArgumentOutOfRangeException("ptr", "Value, startIndex and length do not refer to a valid string.");
					}
					memcpy(dest, (byte*)(value + startIndex), length);
				}
				catch (NullReferenceException)
				{
					throw new ArgumentOutOfRangeException("ptr", "Value, startIndex and length do not refer to a valid string.");
				}
			}
		}
		return enc.GetString(array);
	}
}
