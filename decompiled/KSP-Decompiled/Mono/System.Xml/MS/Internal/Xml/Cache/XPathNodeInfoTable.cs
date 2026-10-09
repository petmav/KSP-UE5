using System.Text;
using System.Xml.XPath;

namespace MS.Internal.Xml.Cache;

internal sealed class XPathNodeInfoTable
{
	private XPathNodeInfoAtom[] hashTable;

	private int sizeTable;

	private XPathNodeInfoAtom infoCached;

	private const int DefaultTableSize = 32;

	public XPathNodeInfoTable()
	{
		hashTable = new XPathNodeInfoAtom[32];
		sizeTable = 0;
	}

	public XPathNodeInfoAtom Create(string localName, string namespaceUri, string prefix, string baseUri, XPathNode[] pageParent, XPathNode[] pageSibling, XPathNode[] pageSimilar, XPathDocument doc, int lineNumBase, int linePosBase)
	{
		XPathNodeInfoAtom xPathNodeInfoAtom;
		if (infoCached == null)
		{
			xPathNodeInfoAtom = new XPathNodeInfoAtom(localName, namespaceUri, prefix, baseUri, pageParent, pageSibling, pageSimilar, doc, lineNumBase, linePosBase);
		}
		else
		{
			xPathNodeInfoAtom = infoCached;
			infoCached = xPathNodeInfoAtom.Next;
			xPathNodeInfoAtom.Init(localName, namespaceUri, prefix, baseUri, pageParent, pageSibling, pageSimilar, doc, lineNumBase, linePosBase);
		}
		return Atomize(xPathNodeInfoAtom);
	}

	private XPathNodeInfoAtom Atomize(XPathNodeInfoAtom info)
	{
		for (XPathNodeInfoAtom xPathNodeInfoAtom = hashTable[info.GetHashCode() & (hashTable.Length - 1)]; xPathNodeInfoAtom != null; xPathNodeInfoAtom = xPathNodeInfoAtom.Next)
		{
			if (info.Equals(xPathNodeInfoAtom))
			{
				info.Next = infoCached;
				infoCached = info;
				return xPathNodeInfoAtom;
			}
		}
		if (sizeTable >= hashTable.Length)
		{
			XPathNodeInfoAtom[] array = hashTable;
			hashTable = new XPathNodeInfoAtom[array.Length * 2];
			for (int i = 0; i < array.Length; i++)
			{
				XPathNodeInfoAtom xPathNodeInfoAtom = array[i];
				while (xPathNodeInfoAtom != null)
				{
					XPathNodeInfoAtom next = xPathNodeInfoAtom.Next;
					AddInfo(xPathNodeInfoAtom);
					xPathNodeInfoAtom = next;
				}
			}
		}
		AddInfo(info);
		return info;
	}

	private void AddInfo(XPathNodeInfoAtom info)
	{
		int num = info.GetHashCode() & (hashTable.Length - 1);
		info.Next = hashTable[num];
		hashTable[num] = info;
		sizeTable++;
	}

	public override string ToString()
	{
		StringBuilder stringBuilder = new StringBuilder();
		for (int i = 0; i < hashTable.Length; i++)
		{
			stringBuilder.AppendFormat("{0,4}: ", i);
			for (XPathNodeInfoAtom xPathNodeInfoAtom = hashTable[i]; xPathNodeInfoAtom != null; xPathNodeInfoAtom = xPathNodeInfoAtom.Next)
			{
				if (xPathNodeInfoAtom != hashTable[i])
				{
					stringBuilder.Append("\n      ");
				}
				stringBuilder.Append(xPathNodeInfoAtom);
			}
			stringBuilder.Append('\n');
		}
		return stringBuilder.ToString();
	}
}
