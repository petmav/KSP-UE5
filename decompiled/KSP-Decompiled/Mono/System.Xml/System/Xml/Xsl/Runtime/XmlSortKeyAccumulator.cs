using System.ComponentModel;

namespace System.Xml.Xsl.Runtime;

[EditorBrowsable(EditorBrowsableState.Never)]
public struct XmlSortKeyAccumulator
{
	private XmlSortKey[] keys;

	private int pos;

	private const int DefaultSortKeyCount = 64;

	public Array Keys => keys;

	public void Create()
	{
		if (keys == null)
		{
			keys = new XmlSortKey[64];
		}
		pos = 0;
		keys[0] = null;
	}

	public void AddStringSortKey(XmlCollation collation, string value)
	{
		AppendSortKey(collation.CreateSortKey(value));
	}

	public void AddDecimalSortKey(XmlCollation collation, decimal value)
	{
		AppendSortKey(new XmlDecimalSortKey(value, collation));
	}

	public void AddIntegerSortKey(XmlCollation collation, long value)
	{
		AppendSortKey(new XmlIntegerSortKey(value, collation));
	}

	public void AddIntSortKey(XmlCollation collation, int value)
	{
		AppendSortKey(new XmlIntSortKey(value, collation));
	}

	public void AddDoubleSortKey(XmlCollation collation, double value)
	{
		AppendSortKey(new XmlDoubleSortKey(value, collation));
	}

	public void AddDateTimeSortKey(XmlCollation collation, DateTime value)
	{
		AppendSortKey(new XmlDateTimeSortKey(value, collation));
	}

	public void AddEmptySortKey(XmlCollation collation)
	{
		AppendSortKey(new XmlEmptySortKey(collation));
	}

	public void FinishSortKeys()
	{
		pos++;
		if (pos >= keys.Length)
		{
			XmlSortKey[] destinationArray = new XmlSortKey[pos * 2];
			Array.Copy(keys, 0, destinationArray, 0, keys.Length);
			keys = destinationArray;
		}
		keys[pos] = null;
	}

	private void AppendSortKey(XmlSortKey key)
	{
		key.Priority = pos;
		if (keys[pos] == null)
		{
			keys[pos] = key;
		}
		else
		{
			keys[pos].AddSortKey(key);
		}
	}
}
