using System.Collections.Generic;

namespace System.Xml.Xsl.IlGen;

internal class UniqueList<T>
{
	private Dictionary<T, int> lookup = new Dictionary<T, int>();

	private List<T> list = new List<T>();

	public int Add(T value)
	{
		int num;
		if (!lookup.ContainsKey(value))
		{
			num = list.Count;
			lookup.Add(value, num);
			list.Add(value);
		}
		else
		{
			num = lookup[value];
		}
		return num;
	}

	public T[] ToArray()
	{
		return list.ToArray();
	}
}
