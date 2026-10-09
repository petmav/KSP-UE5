using System.Collections;
using System.Collections.Generic;

namespace System;

[Serializable]
public struct ArraySegment<T> : IList<T>, ICollection<T>, IEnumerable<T>, IEnumerable, IReadOnlyList<T>, IReadOnlyCollection<T>
{
	[Serializable]
	private sealed class ArraySegmentEnumerator : IEnumerator<T>, IDisposable, IEnumerator
	{
		private T[] _array;

		private int _start;

		private int _end;

		private int _current;

		public T Current
		{
			get
			{
				if (_current < _start)
				{
					throw new InvalidOperationException(Environment.GetResourceString("Enumeration has not started. Call MoveNext."));
				}
				if (_current >= _end)
				{
					throw new InvalidOperationException(Environment.GetResourceString("Enumeration already finished."));
				}
				return _array[_current];
			}
		}

		object IEnumerator.Current => Current;

		internal ArraySegmentEnumerator(ArraySegment<T> arraySegment)
		{
			_array = arraySegment._array;
			_start = arraySegment._offset;
			_end = _start + arraySegment._count;
			_current = _start - 1;
		}

		public bool MoveNext()
		{
			if (_current < _end)
			{
				_current++;
				return _current < _end;
			}
			return false;
		}

		void IEnumerator.Reset()
		{
			_current = _start - 1;
		}

		public void Dispose()
		{
		}
	}

	private T[] _array;

	private int _offset;

	private int _count;

	public T[] Array => _array;

	public int Offset => _offset;

	public int Count => _count;

	T IList<T>.this[int index]
	{
		get
		{
			if (_array == null)
			{
				throw new InvalidOperationException(Environment.GetResourceString("The underlying array is null."));
			}
			if (index < 0 || index >= _count)
			{
				throw new ArgumentOutOfRangeException("index");
			}
			return _array[_offset + index];
		}
		set
		{
			if (_array == null)
			{
				throw new InvalidOperationException(Environment.GetResourceString("The underlying array is null."));
			}
			if (index < 0 || index >= _count)
			{
				throw new ArgumentOutOfRangeException("index");
			}
			_array[_offset + index] = value;
		}
	}

	T IReadOnlyList<T>.this[int index]
	{
		get
		{
			if (_array == null)
			{
				throw new InvalidOperationException(Environment.GetResourceString("The underlying array is null."));
			}
			if (index < 0 || index >= _count)
			{
				throw new ArgumentOutOfRangeException("index");
			}
			return _array[_offset + index];
		}
	}

	bool ICollection<T>.IsReadOnly => true;

	public ArraySegment(T[] array)
	{
		if (array == null)
		{
			throw new ArgumentNullException("array");
		}
		_array = array;
		_offset = 0;
		_count = array.Length;
	}

	public ArraySegment(T[] array, int offset, int count)
	{
		if (array == null)
		{
			throw new ArgumentNullException("array");
		}
		if (offset < 0)
		{
			throw new ArgumentOutOfRangeException("offset", Environment.GetResourceString("Non-negative number required."));
		}
		if (count < 0)
		{
			throw new ArgumentOutOfRangeException("count", Environment.GetResourceString("Non-negative number required."));
		}
		if (array.Length - offset < count)
		{
			throw new ArgumentException(Environment.GetResourceString("Offset and length were out of bounds for the array or count is greater than the number of elements from index to the end of the source collection."));
		}
		_array = array;
		_offset = offset;
		_count = count;
	}

	public override int GetHashCode()
	{
		if (_array != null)
		{
			return _array.GetHashCode() ^ _offset ^ _count;
		}
		return 0;
	}

	public override bool Equals(object obj)
	{
		if (obj is ArraySegment<T>)
		{
			return Equals((ArraySegment<T>)obj);
		}
		return false;
	}

	public bool Equals(ArraySegment<T> obj)
	{
		if (obj._array == _array && obj._offset == _offset)
		{
			return obj._count == _count;
		}
		return false;
	}

	public static bool operator ==(ArraySegment<T> a, ArraySegment<T> b)
	{
		return a.Equals(b);
	}

	public static bool operator !=(ArraySegment<T> a, ArraySegment<T> b)
	{
		return !(a == b);
	}

	int IList<T>.IndexOf(T item)
	{
		if (_array == null)
		{
			throw new InvalidOperationException(Environment.GetResourceString("The underlying array is null."));
		}
		int num = System.Array.IndexOf(_array, item, _offset, _count);
		if (num < 0)
		{
			return -1;
		}
		return num - _offset;
	}

	void IList<T>.Insert(int index, T item)
	{
		throw new NotSupportedException();
	}

	void IList<T>.RemoveAt(int index)
	{
		throw new NotSupportedException();
	}

	void ICollection<T>.Add(T item)
	{
		throw new NotSupportedException();
	}

	void ICollection<T>.Clear()
	{
		throw new NotSupportedException();
	}

	bool ICollection<T>.Contains(T item)
	{
		if (_array == null)
		{
			throw new InvalidOperationException(Environment.GetResourceString("The underlying array is null."));
		}
		return System.Array.IndexOf(_array, item, _offset, _count) >= 0;
	}

	void ICollection<T>.CopyTo(T[] array, int arrayIndex)
	{
		if (_array == null)
		{
			throw new InvalidOperationException(Environment.GetResourceString("The underlying array is null."));
		}
		System.Array.Copy(_array, _offset, array, arrayIndex, _count);
	}

	bool ICollection<T>.Remove(T item)
	{
		throw new NotSupportedException();
	}

	IEnumerator<T> IEnumerable<T>.GetEnumerator()
	{
		if (_array == null)
		{
			throw new InvalidOperationException(Environment.GetResourceString("The underlying array is null."));
		}
		return new ArraySegmentEnumerator(this);
	}

	IEnumerator IEnumerable.GetEnumerator()
	{
		if (_array == null)
		{
			throw new InvalidOperationException(Environment.GetResourceString("The underlying array is null."));
		}
		return new ArraySegmentEnumerator(this);
	}
}
