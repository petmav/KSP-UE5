using System.Collections.Generic;
using System.Xml.Xsl.Qil;
using System.Xml.Xsl.Runtime;

namespace System.Xml.Xsl.IlGen;

internal class StaticDataManager
{
	private UniqueList<string> uniqueNames;

	private UniqueList<Int32Pair> uniqueFilters;

	private List<StringPair[]> prefixMappingsList;

	private List<string> globalNames;

	private UniqueList<EarlyBoundInfo> earlyInfo;

	private UniqueList<XmlQueryType> uniqueXmlTypes;

	private UniqueList<XmlCollation> uniqueCollations;

	public string[] Names
	{
		get
		{
			if (uniqueNames == null)
			{
				return null;
			}
			return uniqueNames.ToArray();
		}
	}

	public Int32Pair[] NameFilters
	{
		get
		{
			if (uniqueFilters == null)
			{
				return null;
			}
			return uniqueFilters.ToArray();
		}
	}

	public StringPair[][] PrefixMappingsList
	{
		get
		{
			if (prefixMappingsList == null)
			{
				return null;
			}
			return prefixMappingsList.ToArray();
		}
	}

	public string[] GlobalNames
	{
		get
		{
			if (globalNames == null)
			{
				return null;
			}
			return globalNames.ToArray();
		}
	}

	public EarlyBoundInfo[] EarlyBound
	{
		get
		{
			if (earlyInfo != null)
			{
				return earlyInfo.ToArray();
			}
			return null;
		}
	}

	public XmlQueryType[] XmlTypes
	{
		get
		{
			if (uniqueXmlTypes == null)
			{
				return null;
			}
			return uniqueXmlTypes.ToArray();
		}
	}

	public XmlCollation[] Collations
	{
		get
		{
			if (uniqueCollations == null)
			{
				return null;
			}
			return uniqueCollations.ToArray();
		}
	}

	public int DeclareName(string name)
	{
		if (uniqueNames == null)
		{
			uniqueNames = new UniqueList<string>();
		}
		return uniqueNames.Add(name);
	}

	public int DeclareNameFilter(string locName, string nsUri)
	{
		if (uniqueFilters == null)
		{
			uniqueFilters = new UniqueList<Int32Pair>();
		}
		return uniqueFilters.Add(new Int32Pair(DeclareName(locName), DeclareName(nsUri)));
	}

	public int DeclarePrefixMappings(IList<QilNode> list)
	{
		StringPair[] array = new StringPair[list.Count];
		for (int i = 0; i < list.Count; i++)
		{
			QilBinary qilBinary = (QilBinary)list[i];
			array[i] = new StringPair((QilLiteral)qilBinary.Left, (QilLiteral)qilBinary.Right);
		}
		if (prefixMappingsList == null)
		{
			prefixMappingsList = new List<StringPair[]>();
		}
		prefixMappingsList.Add(array);
		return prefixMappingsList.Count - 1;
	}

	public int DeclareGlobalValue(string name)
	{
		if (globalNames == null)
		{
			globalNames = new List<string>();
		}
		int count = globalNames.Count;
		globalNames.Add(name);
		return count;
	}

	public int DeclareEarlyBound(string namespaceUri, Type ebType)
	{
		if (earlyInfo == null)
		{
			earlyInfo = new UniqueList<EarlyBoundInfo>();
		}
		return earlyInfo.Add(new EarlyBoundInfo(namespaceUri, ebType));
	}

	public int DeclareXmlType(XmlQueryType type)
	{
		if (uniqueXmlTypes == null)
		{
			uniqueXmlTypes = new UniqueList<XmlQueryType>();
		}
		return uniqueXmlTypes.Add(type);
	}

	public int DeclareCollation(string collation)
	{
		if (uniqueCollations == null)
		{
			uniqueCollations = new UniqueList<XmlCollation>();
		}
		return uniqueCollations.Add(XmlCollation.Create(collation));
	}
}
