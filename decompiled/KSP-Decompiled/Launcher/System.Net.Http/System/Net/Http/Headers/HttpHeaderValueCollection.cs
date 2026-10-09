using System.Collections;
using System.Collections.Generic;
using Unity;

namespace System.Net.Http.Headers;

public sealed class HttpHeaderValueCollection<T> : ICollection<T>, IEnumerable<T>, IEnumerable where T : class
{
	private readonly List<T> list;

	private readonly HttpHeaders headers;

	private readonly HeaderInfo headerInfo;

	private List<string> invalidValues;

	public int Count => list.Count;

	internal List<string> InvalidValues => invalidValues;

	public bool IsReadOnly => false;

	internal HttpHeaderValueCollection(HttpHeaders headers, HeaderInfo headerInfo)
	{
		list = new List<T>();
		this.headers = headers;
		this.headerInfo = headerInfo;
	}

	public void Add(T item)
	{
		list.Add(item);
	}

	internal void AddRange(List<T> values)
	{
		list.AddRange(values);
	}

	internal void AddInvalidValue(string invalidValue)
	{
		if (invalidValues == null)
		{
			invalidValues = new List<string>();
		}
		invalidValues.Add(invalidValue);
	}

	public void Clear()
	{
		list.Clear();
		invalidValues = null;
	}

	public bool Contains(T item)
	{
		return list.Contains(item);
	}

	public void CopyTo(T[] array, int arrayIndex)
	{
		list.CopyTo(array, arrayIndex);
	}

	public void ParseAdd(string input)
	{
		headers.AddValue(input, headerInfo, ignoreInvalid: false);
	}

	public bool Remove(T item)
	{
		return list.Remove(item);
	}

	public override string ToString()
	{
		string text = string.Join(headerInfo.Separator, list);
		if (invalidValues != null)
		{
			text += string.Join(headerInfo.Separator, invalidValues);
		}
		return text;
	}

	public bool TryParseAdd(string input)
	{
		return headers.AddValue(input, headerInfo, ignoreInvalid: true);
	}

	public IEnumerator<T> GetEnumerator()
	{
		return list.GetEnumerator();
	}

	IEnumerator IEnumerable.GetEnumerator()
	{
		return GetEnumerator();
	}

	internal T Find(Predicate<T> predicate)
	{
		return list.Find(predicate);
	}

	internal void Remove(Predicate<T> predicate)
	{
		T val = Find(predicate);
		if (val != null)
		{
			Remove(val);
		}
	}

	internal HttpHeaderValueCollection()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
