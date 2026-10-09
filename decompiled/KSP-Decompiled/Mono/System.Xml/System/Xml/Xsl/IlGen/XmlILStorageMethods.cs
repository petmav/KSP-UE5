using System.Collections.Generic;
using System.Reflection;
using System.Xml.XPath;
using System.Xml.Xsl.Runtime;

namespace System.Xml.Xsl.IlGen;

internal class XmlILStorageMethods
{
	public MethodInfo AggAvg;

	public MethodInfo AggAvgResult;

	public MethodInfo AggCreate;

	public MethodInfo AggIsEmpty;

	public MethodInfo AggMax;

	public MethodInfo AggMaxResult;

	public MethodInfo AggMin;

	public MethodInfo AggMinResult;

	public MethodInfo AggSum;

	public MethodInfo AggSumResult;

	public Type SeqType;

	public FieldInfo SeqEmpty;

	public MethodInfo SeqReuse;

	public MethodInfo SeqReuseSgl;

	public MethodInfo SeqAdd;

	public MethodInfo SeqSortByKeys;

	public Type IListType;

	public MethodInfo IListCount;

	public MethodInfo IListItem;

	public MethodInfo ValueAs;

	public MethodInfo ToAtomicValue;

	public XmlILStorageMethods(Type storageType)
	{
		if (storageType == typeof(int) || storageType == typeof(long) || storageType == typeof(decimal) || storageType == typeof(double))
		{
			Type type = Type.GetType("System.Xml.Xsl.Runtime." + storageType.Name + "Aggregator");
			AggAvg = XmlILMethods.GetMethod(type, "Average");
			AggAvgResult = XmlILMethods.GetMethod(type, "get_AverageResult");
			AggCreate = XmlILMethods.GetMethod(type, "Create");
			AggIsEmpty = XmlILMethods.GetMethod(type, "get_IsEmpty");
			AggMax = XmlILMethods.GetMethod(type, "Maximum");
			AggMaxResult = XmlILMethods.GetMethod(type, "get_MaximumResult");
			AggMin = XmlILMethods.GetMethod(type, "Minimum");
			AggMinResult = XmlILMethods.GetMethod(type, "get_MinimumResult");
			AggSum = XmlILMethods.GetMethod(type, "Sum");
			AggSumResult = XmlILMethods.GetMethod(type, "get_SumResult");
		}
		if (storageType == typeof(XPathNavigator))
		{
			SeqType = typeof(XmlQueryNodeSequence);
			SeqAdd = XmlILMethods.GetMethod(SeqType, "AddClone");
		}
		else if (storageType == typeof(XPathItem))
		{
			SeqType = typeof(XmlQueryItemSequence);
			SeqAdd = XmlILMethods.GetMethod(SeqType, "AddClone");
		}
		else
		{
			SeqType = typeof(XmlQuerySequence<>).MakeGenericType(storageType);
			SeqAdd = XmlILMethods.GetMethod(SeqType, "Add");
		}
		SeqEmpty = SeqType.GetField("Empty");
		SeqReuse = XmlILMethods.GetMethod(SeqType, "CreateOrReuse", SeqType);
		SeqReuseSgl = XmlILMethods.GetMethod(SeqType, "CreateOrReuse", SeqType, storageType);
		SeqSortByKeys = XmlILMethods.GetMethod(SeqType, "SortByKeys");
		IListType = typeof(IList<>).MakeGenericType(storageType);
		IListItem = XmlILMethods.GetMethod(IListType, "get_Item");
		IListCount = XmlILMethods.GetMethod(typeof(ICollection<>).MakeGenericType(storageType), "get_Count");
		if (storageType == typeof(string))
		{
			ValueAs = XmlILMethods.GetMethod(typeof(XPathItem), "get_Value");
		}
		else if (storageType == typeof(int))
		{
			ValueAs = XmlILMethods.GetMethod(typeof(XPathItem), "get_ValueAsInt");
		}
		else if (storageType == typeof(long))
		{
			ValueAs = XmlILMethods.GetMethod(typeof(XPathItem), "get_ValueAsLong");
		}
		else if (storageType == typeof(DateTime))
		{
			ValueAs = XmlILMethods.GetMethod(typeof(XPathItem), "get_ValueAsDateTime");
		}
		else if (storageType == typeof(double))
		{
			ValueAs = XmlILMethods.GetMethod(typeof(XPathItem), "get_ValueAsDouble");
		}
		else if (storageType == typeof(bool))
		{
			ValueAs = XmlILMethods.GetMethod(typeof(XPathItem), "get_ValueAsBoolean");
		}
		if (storageType == typeof(byte[]))
		{
			ToAtomicValue = XmlILMethods.GetMethod(typeof(XmlILStorageConverter), "BytesToAtomicValue");
		}
		else if (storageType != typeof(XPathItem) && storageType != typeof(XPathNavigator))
		{
			ToAtomicValue = XmlILMethods.GetMethod(typeof(XmlILStorageConverter), storageType.Name + "ToAtomicValue");
		}
	}
}
