using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace System.Net.Http.Headers;

internal static class CollectionExtensions
{
	public static bool SequenceEqual<TSource>(this List<TSource> first, List<TSource> second)
	{
		if (first == null)
		{
			if (second != null)
			{
				return second.Count == 0;
			}
			return true;
		}
		if (second == null)
		{
			if (first != null)
			{
				return first.Count == 0;
			}
			return true;
		}
		return Enumerable.SequenceEqual(first, second);
	}

	public static void SetValue(this List<NameValueHeaderValue> parameters, string key, string value)
	{
		for (int i = 0; i < parameters.Count; i++)
		{
			if (string.Equals(parameters[i].Name, key, StringComparison.OrdinalIgnoreCase))
			{
				if (value == null)
				{
					parameters.RemoveAt(i);
				}
				else
				{
					parameters[i].Value = value;
				}
				return;
			}
		}
		if (!string.IsNullOrEmpty(value))
		{
			parameters.Add(new NameValueHeaderValue(key, value));
		}
	}

	public static string ToString<T>(this List<T> list)
	{
		if (list == null || list.Count == 0)
		{
			return null;
		}
		StringBuilder stringBuilder = new StringBuilder();
		for (int i = 0; i < list.Count; i++)
		{
			stringBuilder.Append("; ");
			stringBuilder.Append(list[i]);
		}
		return stringBuilder.ToString();
	}

	public static void ToStringBuilder<T>(this List<T> list, StringBuilder sb)
	{
		if (list == null || list.Count == 0)
		{
			return;
		}
		for (int i = 0; i < list.Count; i++)
		{
			if (i > 0)
			{
				sb.Append(", ");
			}
			sb.Append(list[i]);
		}
	}
}
