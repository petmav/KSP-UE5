using System.Collections.Generic;
using System.IO;
using System.Xml.Xsl.IlGen;
using System.Xml.Xsl.Qil;

namespace System.Xml.Xsl.Runtime;

internal class XmlQueryStaticData
{
	public const string DataFieldName = "staticData";

	public const string TypesFieldName = "ebTypes";

	private const int CurrentFormatVersion = 0;

	private XmlWriterSettings defaultWriterSettings;

	private IList<WhitespaceRule> whitespaceRules;

	private string[] names;

	private StringPair[][] prefixMappingsList;

	private Int32Pair[] filters;

	private XmlQueryType[] types;

	private XmlCollation[] collations;

	private string[] globalNames;

	private EarlyBoundInfo[] earlyBound;

	public XmlWriterSettings DefaultWriterSettings => defaultWriterSettings;

	public IList<WhitespaceRule> WhitespaceRules => whitespaceRules;

	public string[] Names => names;

	public StringPair[][] PrefixMappingsList => prefixMappingsList;

	public Int32Pair[] Filters => filters;

	public XmlQueryType[] Types => types;

	public XmlCollation[] Collations => collations;

	public string[] GlobalNames => globalNames;

	public EarlyBoundInfo[] EarlyBound => earlyBound;

	public XmlQueryStaticData(XmlWriterSettings defaultWriterSettings, IList<WhitespaceRule> whitespaceRules, StaticDataManager staticData)
	{
		this.defaultWriterSettings = defaultWriterSettings;
		this.whitespaceRules = whitespaceRules;
		names = staticData.Names;
		prefixMappingsList = staticData.PrefixMappingsList;
		filters = staticData.NameFilters;
		types = staticData.XmlTypes;
		collations = staticData.Collations;
		globalNames = staticData.GlobalNames;
		earlyBound = staticData.EarlyBound;
	}

	public XmlQueryStaticData(byte[] data, Type[] ebTypes)
	{
		XmlQueryDataReader xmlQueryDataReader = new XmlQueryDataReader(new MemoryStream(data, writable: false));
		if ((xmlQueryDataReader.ReadInt32Encoded() & -256) > 0)
		{
			throw new NotSupportedException();
		}
		defaultWriterSettings = new XmlWriterSettings(xmlQueryDataReader);
		int num = xmlQueryDataReader.ReadInt32();
		if (num != 0)
		{
			whitespaceRules = new WhitespaceRule[num];
			for (int i = 0; i < num; i++)
			{
				whitespaceRules[i] = new WhitespaceRule(xmlQueryDataReader);
			}
		}
		num = xmlQueryDataReader.ReadInt32();
		if (num != 0)
		{
			names = new string[num];
			for (int j = 0; j < num; j++)
			{
				names[j] = xmlQueryDataReader.ReadString();
			}
		}
		num = xmlQueryDataReader.ReadInt32();
		if (num != 0)
		{
			prefixMappingsList = new StringPair[num][];
			for (int k = 0; k < num; k++)
			{
				int num2 = xmlQueryDataReader.ReadInt32();
				prefixMappingsList[k] = new StringPair[num2];
				for (int l = 0; l < num2; l++)
				{
					prefixMappingsList[k][l] = new StringPair(xmlQueryDataReader.ReadString(), xmlQueryDataReader.ReadString());
				}
			}
		}
		num = xmlQueryDataReader.ReadInt32();
		if (num != 0)
		{
			filters = new Int32Pair[num];
			for (int m = 0; m < num; m++)
			{
				filters[m] = new Int32Pair(xmlQueryDataReader.ReadInt32Encoded(), xmlQueryDataReader.ReadInt32Encoded());
			}
		}
		num = xmlQueryDataReader.ReadInt32();
		if (num != 0)
		{
			types = new XmlQueryType[num];
			for (int n = 0; n < num; n++)
			{
				types[n] = XmlQueryTypeFactory.Deserialize(xmlQueryDataReader);
			}
		}
		num = xmlQueryDataReader.ReadInt32();
		if (num != 0)
		{
			collations = new XmlCollation[num];
			for (int num3 = 0; num3 < num; num3++)
			{
				collations[num3] = new XmlCollation(xmlQueryDataReader);
			}
		}
		num = xmlQueryDataReader.ReadInt32();
		if (num != 0)
		{
			globalNames = new string[num];
			for (int num4 = 0; num4 < num; num4++)
			{
				globalNames[num4] = xmlQueryDataReader.ReadString();
			}
		}
		num = xmlQueryDataReader.ReadInt32();
		if (num != 0)
		{
			earlyBound = new EarlyBoundInfo[num];
			for (int num5 = 0; num5 < num; num5++)
			{
				earlyBound[num5] = new EarlyBoundInfo(xmlQueryDataReader.ReadString(), ebTypes[num5]);
			}
		}
		xmlQueryDataReader.Close();
	}

	public void GetObjectData(out byte[] data, out Type[] ebTypes)
	{
		MemoryStream memoryStream = new MemoryStream(4096);
		XmlQueryDataWriter xmlQueryDataWriter = new XmlQueryDataWriter(memoryStream);
		xmlQueryDataWriter.WriteInt32Encoded(0);
		defaultWriterSettings.GetObjectData(xmlQueryDataWriter);
		if (whitespaceRules == null)
		{
			xmlQueryDataWriter.Write(0);
		}
		else
		{
			xmlQueryDataWriter.Write(whitespaceRules.Count);
			foreach (WhitespaceRule whitespaceRule in whitespaceRules)
			{
				whitespaceRule.GetObjectData(xmlQueryDataWriter);
			}
		}
		if (names == null)
		{
			xmlQueryDataWriter.Write(0);
		}
		else
		{
			xmlQueryDataWriter.Write(names.Length);
			string[] array = names;
			foreach (string value in array)
			{
				xmlQueryDataWriter.Write(value);
			}
		}
		if (prefixMappingsList == null)
		{
			xmlQueryDataWriter.Write(0);
		}
		else
		{
			xmlQueryDataWriter.Write(prefixMappingsList.Length);
			StringPair[][] array2 = prefixMappingsList;
			foreach (StringPair[] array3 in array2)
			{
				xmlQueryDataWriter.Write(array3.Length);
				StringPair[] array4 = array3;
				for (int j = 0; j < array4.Length; j++)
				{
					StringPair stringPair = array4[j];
					xmlQueryDataWriter.Write(stringPair.Left);
					xmlQueryDataWriter.Write(stringPair.Right);
				}
			}
		}
		if (filters == null)
		{
			xmlQueryDataWriter.Write(0);
		}
		else
		{
			xmlQueryDataWriter.Write(filters.Length);
			Int32Pair[] array5 = filters;
			for (int i = 0; i < array5.Length; i++)
			{
				Int32Pair int32Pair = array5[i];
				xmlQueryDataWriter.WriteInt32Encoded(int32Pair.Left);
				xmlQueryDataWriter.WriteInt32Encoded(int32Pair.Right);
			}
		}
		if (types == null)
		{
			xmlQueryDataWriter.Write(0);
		}
		else
		{
			xmlQueryDataWriter.Write(types.Length);
			XmlQueryType[] array6 = types;
			foreach (XmlQueryType type in array6)
			{
				XmlQueryTypeFactory.Serialize(xmlQueryDataWriter, type);
			}
		}
		if (collations == null)
		{
			xmlQueryDataWriter.Write(0);
		}
		else
		{
			xmlQueryDataWriter.Write(collations.Length);
			XmlCollation[] array7 = collations;
			for (int i = 0; i < array7.Length; i++)
			{
				array7[i].GetObjectData(xmlQueryDataWriter);
			}
		}
		if (globalNames == null)
		{
			xmlQueryDataWriter.Write(0);
		}
		else
		{
			xmlQueryDataWriter.Write(globalNames.Length);
			string[] array = globalNames;
			foreach (string value2 in array)
			{
				xmlQueryDataWriter.Write(value2);
			}
		}
		if (earlyBound == null)
		{
			xmlQueryDataWriter.Write(0);
			ebTypes = null;
		}
		else
		{
			xmlQueryDataWriter.Write(earlyBound.Length);
			ebTypes = new Type[earlyBound.Length];
			int num = 0;
			EarlyBoundInfo[] array8 = earlyBound;
			foreach (EarlyBoundInfo earlyBoundInfo in array8)
			{
				xmlQueryDataWriter.Write(earlyBoundInfo.NamespaceUri);
				ebTypes[num++] = earlyBoundInfo.EarlyBoundType;
			}
		}
		xmlQueryDataWriter.Close();
		data = memoryStream.ToArray();
	}
}
