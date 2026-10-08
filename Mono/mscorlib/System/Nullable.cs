using System.Collections.Generic;
using System.Diagnostics;
using System.Runtime.InteropServices;

namespace System;

[ComVisible(true)]
public static class Nullable
{
	public static int Compare<T>(T? n1, T? n2) where T : struct
	{
		if (n1.has_value)
		{
			if (!n2.has_value)
			{
				return 1;
			}
			return Comparer<T>.Default.Compare(n1.value, n2.value);
		}
		if (!n2.has_value)
		{
			return 0;
		}
		return -1;
	}

	public static bool Equals<T>(T? n1, T? n2) where T : struct
	{
		if (n1.has_value != n2.has_value)
		{
			return false;
		}
		if (!n1.has_value)
		{
			return true;
		}
		return EqualityComparer<T>.Default.Equals(n1.value, n2.value);
	}

	public static Type GetUnderlyingType(Type nullableType)
	{
		if (nullableType == null)
		{
			throw new ArgumentNullException("nullableType");
		}
		if (!nullableType.IsGenericType || nullableType.IsGenericTypeDefinition || !(nullableType.GetGenericTypeDefinition() == typeof(Nullable<>)))
		{
			return null;
		}
		return nullableType.GetGenericArguments()[0];
	}
}
[Serializable]
[DebuggerStepThrough]
public struct Nullable<T> where T : struct
{
	internal T value;

	internal bool has_value;

	public bool HasValue => has_value;

	public T Value
	{
		get
		{
			if (!has_value)
			{
				throw new InvalidOperationException("Nullable object must have a value.");
			}
			return value;
		}
	}

	public Nullable(T value)
	{
		has_value = true;
		this.value = value;
	}

	public override bool Equals(object other)
	{
		if (other == null)
		{
			return !has_value;
		}
		if (!(other is T?))
		{
			return false;
		}
		return Equals((T?)other);
	}

	private bool Equals(T? other)
	{
		if (other.has_value != has_value)
		{
			return false;
		}
		if (!has_value)
		{
			return true;
		}
		return other.value.Equals(value);
	}

	public override int GetHashCode()
	{
		if (!has_value)
		{
			return 0;
		}
		return value.GetHashCode();
	}

	public T GetValueOrDefault()
	{
		return value;
	}

	public T GetValueOrDefault(T defaultValue)
	{
		if (!has_value)
		{
			return defaultValue;
		}
		return value;
	}

	public override string ToString()
	{
		if (has_value)
		{
			return value.ToString();
		}
		return string.Empty;
	}

	public static implicit operator T?(T value)
	{
		return value;
	}

	public static explicit operator T(T? value)
	{
		return value.Value;
	}

	private static object Box(T? o)
	{
		if (!o.has_value)
		{
			return null;
		}
		return o.value;
	}

	private static T? Unbox(object o)
	{
		if (o == null)
		{
			return null;
		}
		return (T)o;
	}
}
