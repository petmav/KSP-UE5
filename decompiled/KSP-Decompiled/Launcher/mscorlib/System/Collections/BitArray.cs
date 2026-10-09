using System.Runtime.InteropServices;
using System.Threading;

namespace System.Collections;

[Serializable]
[ComVisible(true)]
public sealed class BitArray : ICollection, IEnumerable, ICloneable
{
	[Serializable]
	private class BitArrayEnumeratorSimple : IEnumerator, ICloneable
	{
		private BitArray bitarray;

		private int index;

		private int version;

		private bool currentElement;

		public virtual object Current
		{
			get
			{
				if (index == -1)
				{
					throw new InvalidOperationException(Environment.GetResourceString("Enumeration has not started. Call MoveNext."));
				}
				if (index >= bitarray.Count)
				{
					throw new InvalidOperationException(Environment.GetResourceString("Enumeration already finished."));
				}
				return currentElement;
			}
		}

		internal BitArrayEnumeratorSimple(BitArray bitarray)
		{
			this.bitarray = bitarray;
			index = -1;
			version = bitarray._version;
		}

		public object Clone()
		{
			return MemberwiseClone();
		}

		public virtual bool MoveNext()
		{
			if (version != bitarray._version)
			{
				throw new InvalidOperationException(Environment.GetResourceString("Collection was modified; enumeration operation may not execute."));
			}
			if (index < bitarray.Count - 1)
			{
				index++;
				currentElement = bitarray.Get(index);
				return true;
			}
			index = bitarray.Count;
			return false;
		}

		public void Reset()
		{
			if (version != bitarray._version)
			{
				throw new InvalidOperationException(Environment.GetResourceString("Collection was modified; enumeration operation may not execute."));
			}
			index = -1;
		}
	}

	private const int BitsPerInt32 = 32;

	private const int BytesPerInt32 = 4;

	private const int BitsPerByte = 8;

	private int[] m_array;

	private int m_length;

	private int _version;

	[NonSerialized]
	private object _syncRoot;

	private const int _ShrinkThreshold = 256;

	public bool this[int index]
	{
		get
		{
			return Get(index);
		}
		set
		{
			Set(index, value);
		}
	}

	public int Length
	{
		get
		{
			return m_length;
		}
		set
		{
			if (value < 0)
			{
				throw new ArgumentOutOfRangeException("value", Environment.GetResourceString("Non-negative number required."));
			}
			int arrayLength = GetArrayLength(value, 32);
			if (arrayLength > m_array.Length || arrayLength + 256 < m_array.Length)
			{
				int[] array = new int[arrayLength];
				Array.Copy(m_array, array, (arrayLength > m_array.Length) ? m_array.Length : arrayLength);
				m_array = array;
			}
			if (value > m_length)
			{
				int num = GetArrayLength(m_length, 32) - 1;
				int num2 = m_length % 32;
				if (num2 > 0)
				{
					m_array[num] &= (1 << num2) - 1;
				}
				Array.Clear(m_array, num + 1, arrayLength - num - 1);
			}
			m_length = value;
			_version++;
		}
	}

	public int Count => m_length;

	public object SyncRoot
	{
		get
		{
			if (_syncRoot == null)
			{
				Interlocked.CompareExchange<object>(ref _syncRoot, new object(), (object)null);
			}
			return _syncRoot;
		}
	}

	public bool IsReadOnly => false;

	public bool IsSynchronized => false;

	private BitArray()
	{
	}

	public BitArray(int length)
		: this(length, defaultValue: false)
	{
	}

	public BitArray(int length, bool defaultValue)
	{
		if (length < 0)
		{
			throw new ArgumentOutOfRangeException("length", Environment.GetResourceString("Non-negative number required."));
		}
		m_array = new int[GetArrayLength(length, 32)];
		m_length = length;
		int num = (defaultValue ? (-1) : 0);
		for (int i = 0; i < m_array.Length; i++)
		{
			m_array[i] = num;
		}
		_version = 0;
	}

	public BitArray(byte[] bytes)
	{
		if (bytes == null)
		{
			throw new ArgumentNullException("bytes");
		}
		if (bytes.Length > 268435455)
		{
			throw new ArgumentException(Environment.GetResourceString("The input array length must not exceed Int32.MaxValue / {0}. Otherwise BitArray.Length would exceed Int32.MaxValue.", 8), "bytes");
		}
		m_array = new int[GetArrayLength(bytes.Length, 4)];
		m_length = bytes.Length * 8;
		int num = 0;
		int i;
		for (i = 0; bytes.Length - i >= 4; i += 4)
		{
			m_array[num++] = (bytes[i] & 0xFF) | ((bytes[i + 1] & 0xFF) << 8) | ((bytes[i + 2] & 0xFF) << 16) | ((bytes[i + 3] & 0xFF) << 24);
		}
		switch (bytes.Length - i)
		{
		case 3:
			m_array[num] = (bytes[i + 2] & 0xFF) << 16;
			goto case 2;
		case 2:
			m_array[num] |= (bytes[i + 1] & 0xFF) << 8;
			goto case 1;
		case 1:
			m_array[num] |= bytes[i] & 0xFF;
			break;
		}
		_version = 0;
	}

	public BitArray(bool[] values)
	{
		if (values == null)
		{
			throw new ArgumentNullException("values");
		}
		m_array = new int[GetArrayLength(values.Length, 32)];
		m_length = values.Length;
		for (int i = 0; i < values.Length; i++)
		{
			if (values[i])
			{
				m_array[i / 32] |= 1 << i % 32;
			}
		}
		_version = 0;
	}

	public BitArray(int[] values)
	{
		if (values == null)
		{
			throw new ArgumentNullException("values");
		}
		if (values.Length > 67108863)
		{
			throw new ArgumentException(Environment.GetResourceString("The input array length must not exceed Int32.MaxValue / {0}. Otherwise BitArray.Length would exceed Int32.MaxValue.", 32), "values");
		}
		m_array = new int[values.Length];
		m_length = values.Length * 32;
		Array.Copy(values, m_array, values.Length);
		_version = 0;
	}

	public BitArray(BitArray bits)
	{
		if (bits == null)
		{
			throw new ArgumentNullException("bits");
		}
		int arrayLength = GetArrayLength(bits.m_length, 32);
		m_array = new int[arrayLength];
		m_length = bits.m_length;
		Array.Copy(bits.m_array, m_array, arrayLength);
		_version = bits._version;
	}

	public bool Get(int index)
	{
		if (index < 0 || index >= Length)
		{
			throw new ArgumentOutOfRangeException("index", Environment.GetResourceString("Index was out of range. Must be non-negative and less than the size of the collection."));
		}
		return (m_array[index / 32] & (1 << index % 32)) != 0;
	}

	public void Set(int index, bool value)
	{
		if (index < 0 || index >= Length)
		{
			throw new ArgumentOutOfRangeException("index", Environment.GetResourceString("Index was out of range. Must be non-negative and less than the size of the collection."));
		}
		if (value)
		{
			m_array[index / 32] |= 1 << index % 32;
		}
		else
		{
			m_array[index / 32] &= ~(1 << index % 32);
		}
		_version++;
	}

	public void SetAll(bool value)
	{
		int num = (value ? (-1) : 0);
		int arrayLength = GetArrayLength(m_length, 32);
		for (int i = 0; i < arrayLength; i++)
		{
			m_array[i] = num;
		}
		_version++;
	}

	public BitArray And(BitArray value)
	{
		if (value == null)
		{
			throw new ArgumentNullException("value");
		}
		if (Length != value.Length)
		{
			throw new ArgumentException(Environment.GetResourceString("Array lengths must be the same."));
		}
		int arrayLength = GetArrayLength(m_length, 32);
		for (int i = 0; i < arrayLength; i++)
		{
			m_array[i] &= value.m_array[i];
		}
		_version++;
		return this;
	}

	public BitArray Or(BitArray value)
	{
		if (value == null)
		{
			throw new ArgumentNullException("value");
		}
		if (Length != value.Length)
		{
			throw new ArgumentException(Environment.GetResourceString("Array lengths must be the same."));
		}
		int arrayLength = GetArrayLength(m_length, 32);
		for (int i = 0; i < arrayLength; i++)
		{
			m_array[i] |= value.m_array[i];
		}
		_version++;
		return this;
	}

	public BitArray Xor(BitArray value)
	{
		if (value == null)
		{
			throw new ArgumentNullException("value");
		}
		if (Length != value.Length)
		{
			throw new ArgumentException(Environment.GetResourceString("Array lengths must be the same."));
		}
		int arrayLength = GetArrayLength(m_length, 32);
		for (int i = 0; i < arrayLength; i++)
		{
			m_array[i] ^= value.m_array[i];
		}
		_version++;
		return this;
	}

	public BitArray Not()
	{
		int arrayLength = GetArrayLength(m_length, 32);
		for (int i = 0; i < arrayLength; i++)
		{
			m_array[i] = ~m_array[i];
		}
		_version++;
		return this;
	}

	public void CopyTo(Array array, int index)
	{
		if (array == null)
		{
			throw new ArgumentNullException("array");
		}
		if (index < 0)
		{
			throw new ArgumentOutOfRangeException("index", Environment.GetResourceString("Non-negative number required."));
		}
		if (array.Rank != 1)
		{
			throw new ArgumentException(Environment.GetResourceString("Only single dimensional arrays are supported for the requested action."));
		}
		if (array is int[])
		{
			Array.Copy(m_array, 0, array, index, GetArrayLength(m_length, 32));
			return;
		}
		if (array is byte[])
		{
			int arrayLength = GetArrayLength(m_length, 8);
			if (array.Length - index < arrayLength)
			{
				throw new ArgumentException(Environment.GetResourceString("Offset and length were out of bounds for the array or count is greater than the number of elements from index to the end of the source collection."));
			}
			byte[] array2 = (byte[])array;
			for (int i = 0; i < arrayLength; i++)
			{
				array2[index + i] = (byte)((uint)(m_array[i / 4] >> i % 4 * 8) & 0xFFu);
			}
			return;
		}
		if (array is bool[])
		{
			if (array.Length - index < m_length)
			{
				throw new ArgumentException(Environment.GetResourceString("Offset and length were out of bounds for the array or count is greater than the number of elements from index to the end of the source collection."));
			}
			bool[] array3 = (bool[])array;
			for (int j = 0; j < m_length; j++)
			{
				array3[index + j] = ((m_array[j / 32] >> j % 32) & 1) != 0;
			}
			return;
		}
		throw new ArgumentException(Environment.GetResourceString("Only supported array types for CopyTo on BitArrays are Boolean[], Int32[] and Byte[]."));
	}

	public object Clone()
	{
		return new BitArray(m_array)
		{
			_version = _version,
			m_length = m_length
		};
	}

	public IEnumerator GetEnumerator()
	{
		return new BitArrayEnumeratorSimple(this);
	}

	private static int GetArrayLength(int n, int div)
	{
		if (n <= 0)
		{
			return 0;
		}
		return (n - 1) / div + 1;
	}
}
