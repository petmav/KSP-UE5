using System.Globalization;

namespace System.Xml.Xsl.Runtime;

internal class XmlStringSortKey : XmlSortKey
{
	private SortKey sortKey;

	private byte[] sortKeyBytes;

	private bool descendingOrder;

	public XmlStringSortKey(SortKey sortKey, bool descendingOrder)
	{
		this.sortKey = sortKey;
		this.descendingOrder = descendingOrder;
	}

	public XmlStringSortKey(byte[] sortKey, bool descendingOrder)
	{
		sortKeyBytes = sortKey;
		this.descendingOrder = descendingOrder;
	}

	public override int CompareTo(object obj)
	{
		if (!(obj is XmlStringSortKey xmlStringSortKey))
		{
			return CompareToEmpty(obj);
		}
		int num;
		if (sortKey != null)
		{
			num = SortKey.Compare(sortKey, xmlStringSortKey.sortKey);
		}
		else
		{
			int num2 = ((sortKeyBytes.Length < xmlStringSortKey.sortKeyBytes.Length) ? sortKeyBytes.Length : xmlStringSortKey.sortKeyBytes.Length);
			int num3 = 0;
			while (true)
			{
				if (num3 < num2)
				{
					if (sortKeyBytes[num3] < xmlStringSortKey.sortKeyBytes[num3])
					{
						num = -1;
						break;
					}
					if (sortKeyBytes[num3] > xmlStringSortKey.sortKeyBytes[num3])
					{
						num = 1;
						break;
					}
					num3++;
					continue;
				}
				num = ((sortKeyBytes.Length >= xmlStringSortKey.sortKeyBytes.Length) ? ((sortKeyBytes.Length > xmlStringSortKey.sortKeyBytes.Length) ? 1 : 0) : (-1));
				break;
			}
		}
		if (num == 0)
		{
			return BreakSortingTie(xmlStringSortKey);
		}
		if (!descendingOrder)
		{
			return num;
		}
		return -num;
	}
}
