using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using System.Globalization;
using System.Reflection;
using System.Xml.Schema;
using System.Xml.XPath;
using MS.Internal.Xml.XPath;

namespace System.Xml.Xsl.Runtime;

[EditorBrowsable(EditorBrowsableState.Never)]
public sealed class XmlQueryRuntime
{
	private XmlQueryContext ctxt;

	private XsltLibrary xsltLib;

	private EarlyBoundInfo[] earlyInfo;

	private object[] earlyObjects;

	private string[] globalNames;

	private object[] globalValues;

	private XmlNameTable nameTableQuery;

	private string[] atomizedNames;

	private XmlNavigatorFilter[] filters;

	private StringPair[][] prefixMappingsList;

	private XmlQueryType[] types;

	private XmlCollation[] collations;

	private DocumentOrderComparer docOrderCmp;

	private ArrayList[] indexes;

	private XmlQueryOutput output;

	private Stack<XmlQueryOutput> stkOutput;

	internal const BindingFlags EarlyBoundFlags = BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public;

	internal const BindingFlags LateBoundFlags = BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public;

	public XmlQueryContext ExternalContext => ctxt;

	public XsltLibrary XsltFunctions
	{
		get
		{
			if (xsltLib == null)
			{
				xsltLib = new XsltLibrary(this);
			}
			return xsltLib;
		}
	}

	public XmlNameTable NameTable => nameTableQuery;

	internal XmlQueryType[] XmlTypes => types;

	public XmlQueryOutput Output => output;

	internal XmlQueryRuntime(XmlQueryStaticData data, object defaultDataSource, XmlResolver dataSources, XsltArgumentList argList, XmlSequenceWriter seqWrt)
	{
		string[] names = data.Names;
		Int32Pair[] array = data.Filters;
		ctxt = new XmlQueryContext(this, defaultDataSource, dataSources, argList, (data.WhitespaceRules != null && data.WhitespaceRules.Count != 0) ? new WhitespaceRuleLookup(data.WhitespaceRules) : null);
		xsltLib = null;
		earlyInfo = data.EarlyBound;
		earlyObjects = ((earlyInfo != null) ? new object[earlyInfo.Length] : null);
		globalNames = data.GlobalNames;
		globalValues = ((globalNames != null) ? new object[globalNames.Length] : null);
		nameTableQuery = ctxt.QueryNameTable;
		atomizedNames = null;
		if (names != null)
		{
			XmlNameTable defaultNameTable = ctxt.DefaultNameTable;
			atomizedNames = new string[names.Length];
			if (defaultNameTable != nameTableQuery && defaultNameTable != null)
			{
				for (int i = 0; i < names.Length; i++)
				{
					string text = defaultNameTable.Get(names[i]);
					atomizedNames[i] = nameTableQuery.Add(text ?? names[i]);
				}
			}
			else
			{
				for (int i = 0; i < names.Length; i++)
				{
					atomizedNames[i] = nameTableQuery.Add(names[i]);
				}
			}
		}
		filters = null;
		if (array != null)
		{
			filters = new XmlNavigatorFilter[array.Length];
			for (int i = 0; i < array.Length; i++)
			{
				filters[i] = XmlNavNameFilter.Create(atomizedNames[array[i].Left], atomizedNames[array[i].Right]);
			}
		}
		prefixMappingsList = data.PrefixMappingsList;
		types = data.Types;
		collations = data.Collations;
		docOrderCmp = new DocumentOrderComparer();
		indexes = null;
		stkOutput = new Stack<XmlQueryOutput>(16);
		output = new XmlQueryOutput(this, seqWrt);
	}

	public string[] DebugGetGlobalNames()
	{
		return globalNames;
	}

	public IList DebugGetGlobalValue(string name)
	{
		for (int i = 0; i < globalNames.Length; i++)
		{
			if (globalNames[i] == name)
			{
				return (IList)globalValues[i];
			}
		}
		return null;
	}

	public void DebugSetGlobalValue(string name, object value)
	{
		for (int i = 0; i < globalNames.Length; i++)
		{
			if (globalNames[i] == name)
			{
				globalValues[i] = (IList<XPathItem>)XmlAnyListConverter.ItemList.ChangeType(value, typeof(XPathItem[]), null);
				break;
			}
		}
	}

	public object DebugGetXsltValue(IList seq)
	{
		if (seq != null && seq.Count == 1)
		{
			XPathItem xPathItem = seq[0] as XPathItem;
			if (xPathItem != null && !xPathItem.IsNode)
			{
				return xPathItem.TypedValue;
			}
			if (xPathItem is RtfNavigator)
			{
				return ((RtfNavigator)xPathItem).ToNavigator();
			}
		}
		return seq;
	}

	public object GetEarlyBoundObject(int index)
	{
		object obj = earlyObjects[index];
		if (obj == null)
		{
			obj = earlyInfo[index].CreateObject();
			earlyObjects[index] = obj;
		}
		return obj;
	}

	public bool EarlyBoundFunctionExists(string name, string namespaceUri)
	{
		if (earlyInfo == null)
		{
			return false;
		}
		for (int i = 0; i < earlyInfo.Length; i++)
		{
			if (namespaceUri == earlyInfo[i].NamespaceUri)
			{
				return new XmlExtensionFunction(name, namespaceUri, -1, earlyInfo[i].EarlyBoundType, BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public).CanBind();
			}
		}
		return false;
	}

	public bool IsGlobalComputed(int index)
	{
		return globalValues[index] != null;
	}

	public object GetGlobalValue(int index)
	{
		return globalValues[index];
	}

	public void SetGlobalValue(int index, object value)
	{
		globalValues[index] = value;
	}

	public string GetAtomizedName(int index)
	{
		return atomizedNames[index];
	}

	public XmlNavigatorFilter GetNameFilter(int index)
	{
		return filters[index];
	}

	public XmlNavigatorFilter GetTypeFilter(XPathNodeType nodeType)
	{
		return nodeType switch
		{
			XPathNodeType.All => XmlNavNeverFilter.Create(), 
			XPathNodeType.Attribute => XmlNavAttrFilter.Create(), 
			_ => XmlNavTypeFilter.Create(nodeType), 
		};
	}

	public XmlQualifiedName ParseTagName(string tagName, int indexPrefixMappings)
	{
		ParseTagName(tagName, indexPrefixMappings, out var _, out var localName, out var ns);
		return new XmlQualifiedName(localName, ns);
	}

	public XmlQualifiedName ParseTagName(string tagName, string ns)
	{
		ValidateNames.ParseQNameThrow(tagName, out var _, out var localName);
		return new XmlQualifiedName(localName, ns);
	}

	internal void ParseTagName(string tagName, int idxPrefixMappings, out string prefix, out string localName, out string ns)
	{
		ValidateNames.ParseQNameThrow(tagName, out prefix, out localName);
		ns = null;
		StringPair[] array = prefixMappingsList[idxPrefixMappings];
		for (int i = 0; i < array.Length; i++)
		{
			StringPair stringPair = array[i];
			if (prefix == stringPair.Left)
			{
				ns = stringPair.Right;
				break;
			}
		}
		if (ns != null)
		{
			return;
		}
		if (prefix.Length == 0)
		{
			ns = "";
			return;
		}
		if (prefix.Equals("xml"))
		{
			ns = "http://www.w3.org/XML/1998/namespace";
			return;
		}
		if (prefix.Equals("xmlns"))
		{
			ns = "http://www.w3.org/2000/xmlns/";
			return;
		}
		throw new XslTransformException("Prefix '{0}' is not defined.", prefix);
	}

	public bool IsQNameEqual(XPathNavigator n1, XPathNavigator n2)
	{
		if (n1.NameTable == n2.NameTable)
		{
			if ((object)n1.LocalName == n2.LocalName)
			{
				return (object)n1.NamespaceURI == n2.NamespaceURI;
			}
			return false;
		}
		if (n1.LocalName == n2.LocalName)
		{
			return n1.NamespaceURI == n2.NamespaceURI;
		}
		return false;
	}

	public bool IsQNameEqual(XPathNavigator navigator, int indexLocalName, int indexNamespaceUri)
	{
		if (navigator.NameTable == nameTableQuery)
		{
			if ((object)GetAtomizedName(indexLocalName) == navigator.LocalName)
			{
				return (object)GetAtomizedName(indexNamespaceUri) == navigator.NamespaceURI;
			}
			return false;
		}
		if (GetAtomizedName(indexLocalName) == navigator.LocalName)
		{
			return GetAtomizedName(indexNamespaceUri) == navigator.NamespaceURI;
		}
		return false;
	}

	internal XmlQueryType GetXmlType(int idxType)
	{
		return types[idxType];
	}

	public object ChangeTypeXsltArgument(int indexType, object value, Type destinationType)
	{
		return ChangeTypeXsltArgument(GetXmlType(indexType), value, destinationType);
	}

	internal object ChangeTypeXsltArgument(XmlQueryType xmlType, object value, Type destinationType)
	{
		switch (xmlType.TypeCode)
		{
		case XmlTypeCode.String:
			if (destinationType == XsltConvert.DateTimeType)
			{
				value = XsltConvert.ToDateTime((string)value);
			}
			break;
		case XmlTypeCode.Double:
			if (destinationType != XsltConvert.DoubleType)
			{
				value = Convert.ChangeType(value, destinationType, CultureInfo.InvariantCulture);
			}
			break;
		case XmlTypeCode.Node:
			if (destinationType == XsltConvert.XPathNodeIteratorType)
			{
				value = new XPathArrayIterator((IList)value);
			}
			else if (destinationType == XsltConvert.XPathNavigatorArrayType)
			{
				IList<XPathNavigator> list2 = (IList<XPathNavigator>)value;
				XPathNavigator[] array = new XPathNavigator[list2.Count];
				for (int i = 0; i < list2.Count; i++)
				{
					array[i] = list2[i];
				}
				value = array;
			}
			break;
		case XmlTypeCode.Item:
		{
			if (destinationType != XsltConvert.ObjectType)
			{
				throw new XslTransformException("Extension function parameters or return values which have Clr type '{0}' are not supported.", destinationType.Name);
			}
			IList<XPathItem> list = (IList<XPathItem>)value;
			if (list.Count == 1)
			{
				XPathItem xPathItem = list[0];
				value = ((!xPathItem.IsNode) ? xPathItem.TypedValue : ((!(xPathItem is RtfNavigator rtfNavigator)) ? ((ICloneable)new XPathArrayIterator((IList)value)) : ((ICloneable)rtfNavigator.ToNavigator())));
			}
			else
			{
				value = new XPathArrayIterator((IList)value);
			}
			break;
		}
		}
		return value;
	}

	public object ChangeTypeXsltResult(int indexType, object value)
	{
		return ChangeTypeXsltResult(GetXmlType(indexType), value);
	}

	internal object ChangeTypeXsltResult(XmlQueryType xmlType, object value)
	{
		if (value == null)
		{
			throw new XslTransformException("Extension functions cannot return null values.", string.Empty);
		}
		switch (xmlType.TypeCode)
		{
		case XmlTypeCode.String:
			if (value.GetType() == XsltConvert.DateTimeType)
			{
				value = XsltConvert.ToString((DateTime)value);
			}
			break;
		case XmlTypeCode.Double:
			if (value.GetType() != XsltConvert.DoubleType)
			{
				value = ((IConvertible)value).ToDouble(null);
			}
			break;
		case XmlTypeCode.Node:
			if (xmlType.IsSingleton)
			{
				break;
			}
			if (value is XPathArrayIterator xPathArrayIterator && xPathArrayIterator.AsList is XmlQueryNodeSequence)
			{
				value = xPathArrayIterator.AsList as XmlQueryNodeSequence;
			}
			else
			{
				XmlQueryNodeSequence xmlQueryNodeSequence = new XmlQueryNodeSequence();
				if (value is IList list)
				{
					for (int i = 0; i < list.Count; i++)
					{
						xmlQueryNodeSequence.Add(EnsureNavigator(list[i]));
					}
				}
				else
				{
					foreach (object item in (IEnumerable)value)
					{
						xmlQueryNodeSequence.Add(EnsureNavigator(item));
					}
				}
				value = xmlQueryNodeSequence;
			}
			value = ((XmlQueryNodeSequence)value).DocOrderDistinct(docOrderCmp);
			break;
		case XmlTypeCode.Item:
		{
			Type type = value.GetType();
			switch (XsltConvert.InferXsltType(type).TypeCode)
			{
			case XmlTypeCode.Boolean:
				value = new XmlQueryItemSequence(new XmlAtomicValue(XmlSchemaType.GetBuiltInSimpleType(XmlTypeCode.Boolean), value));
				break;
			case XmlTypeCode.Double:
				value = new XmlQueryItemSequence(new XmlAtomicValue(XmlSchemaType.GetBuiltInSimpleType(XmlTypeCode.Double), ((IConvertible)value).ToDouble(null)));
				break;
			case XmlTypeCode.String:
				value = ((!(type == XsltConvert.DateTimeType)) ? new XmlQueryItemSequence(new XmlAtomicValue(XmlSchemaType.GetBuiltInSimpleType(XmlTypeCode.String), value)) : new XmlQueryItemSequence(new XmlAtomicValue(XmlSchemaType.GetBuiltInSimpleType(XmlTypeCode.String), XsltConvert.ToString((DateTime)value))));
				break;
			case XmlTypeCode.Node:
				value = ChangeTypeXsltResult(XmlQueryTypeFactory.NodeS, value);
				break;
			case XmlTypeCode.Item:
				if (value is XPathNodeIterator)
				{
					value = ChangeTypeXsltResult(XmlQueryTypeFactory.NodeS, value);
					break;
				}
				if (!(value is IXPathNavigable iXPathNavigable))
				{
					throw new XslTransformException("Extension function parameters or return values which have Clr type '{0}' are not supported.", type.Name);
				}
				value = ((!(value is XPathNavigator)) ? new XmlQueryNodeSequence(iXPathNavigable.CreateNavigator()) : new XmlQueryNodeSequence((XPathNavigator)value));
				break;
			}
			break;
		}
		}
		return value;
	}

	private static XPathNavigator EnsureNavigator(object value)
	{
		if (!(value is XPathNavigator result))
		{
			throw new XslTransformException("Extension functions cannot return null values.", string.Empty);
		}
		return result;
	}

	public bool MatchesXmlType(IList<XPathItem> seq, int indexType)
	{
		XmlQueryType xmlType = GetXmlType(indexType);
		if (!(seq.Count switch
		{
			0 => XmlQueryCardinality.Zero, 
			1 => XmlQueryCardinality.One, 
			_ => XmlQueryCardinality.More, 
		} <= xmlType.Cardinality))
		{
			return false;
		}
		xmlType = xmlType.Prime;
		for (int i = 0; i < seq.Count; i++)
		{
			if (!CreateXmlType(seq[0]).IsSubtypeOf(xmlType))
			{
				return false;
			}
		}
		return true;
	}

	public bool MatchesXmlType(XPathItem item, int indexType)
	{
		return CreateXmlType(item).IsSubtypeOf(GetXmlType(indexType));
	}

	public bool MatchesXmlType(IList<XPathItem> seq, XmlTypeCode code)
	{
		if (seq.Count != 1)
		{
			return false;
		}
		return MatchesXmlType(seq[0], code);
	}

	public bool MatchesXmlType(XPathItem item, XmlTypeCode code)
	{
		if (code > XmlTypeCode.AnyAtomicType)
		{
			if (!item.IsNode)
			{
				return item.XmlType.TypeCode == code;
			}
			return false;
		}
		switch (code)
		{
		case XmlTypeCode.AnyAtomicType:
			return !item.IsNode;
		case XmlTypeCode.Node:
			return item.IsNode;
		case XmlTypeCode.Item:
			return true;
		default:
			if (!item.IsNode)
			{
				return false;
			}
			return ((XPathNavigator)item).NodeType switch
			{
				XPathNodeType.Root => code == XmlTypeCode.Document, 
				XPathNodeType.Element => code == XmlTypeCode.Element, 
				XPathNodeType.Attribute => code == XmlTypeCode.Attribute, 
				XPathNodeType.Namespace => code == XmlTypeCode.Namespace, 
				XPathNodeType.Text => code == XmlTypeCode.Text, 
				XPathNodeType.SignificantWhitespace => code == XmlTypeCode.Text, 
				XPathNodeType.Whitespace => code == XmlTypeCode.Text, 
				XPathNodeType.ProcessingInstruction => code == XmlTypeCode.ProcessingInstruction, 
				XPathNodeType.Comment => code == XmlTypeCode.Comment, 
				_ => false, 
			};
		}
	}

	private XmlQueryType CreateXmlType(XPathItem item)
	{
		if (item.IsNode)
		{
			if (item is RtfNavigator)
			{
				return XmlQueryTypeFactory.Node;
			}
			XPathNavigator xPathNavigator = (XPathNavigator)item;
			switch (xPathNavigator.NodeType)
			{
			case XPathNodeType.Root:
			case XPathNodeType.Element:
				if (xPathNavigator.XmlType == null)
				{
					return XmlQueryTypeFactory.Type(xPathNavigator.NodeType, XmlQualifiedNameTest.New(xPathNavigator.LocalName, xPathNavigator.NamespaceURI), XmlSchemaComplexType.UntypedAnyType, isNillable: false);
				}
				return XmlQueryTypeFactory.Type(xPathNavigator.NodeType, XmlQualifiedNameTest.New(xPathNavigator.LocalName, xPathNavigator.NamespaceURI), xPathNavigator.XmlType, xPathNavigator.SchemaInfo.SchemaElement.IsNillable);
			case XPathNodeType.Attribute:
				if (xPathNavigator.XmlType == null)
				{
					return XmlQueryTypeFactory.Type(xPathNavigator.NodeType, XmlQualifiedNameTest.New(xPathNavigator.LocalName, xPathNavigator.NamespaceURI), DatatypeImplementation.UntypedAtomicType, isNillable: false);
				}
				return XmlQueryTypeFactory.Type(xPathNavigator.NodeType, XmlQualifiedNameTest.New(xPathNavigator.LocalName, xPathNavigator.NamespaceURI), xPathNavigator.XmlType, isNillable: false);
			default:
				return XmlQueryTypeFactory.Type(xPathNavigator.NodeType, XmlQualifiedNameTest.Wildcard, XmlSchemaComplexType.AnyType, isNillable: false);
			}
		}
		return XmlQueryTypeFactory.Type((XmlSchemaSimpleType)item.XmlType, isStrict: true);
	}

	public XmlCollation GetCollation(int index)
	{
		return collations[index];
	}

	public XmlCollation CreateCollation(string collation)
	{
		return XmlCollation.Create(collation);
	}

	public int ComparePosition(XPathNavigator navigatorThis, XPathNavigator navigatorThat)
	{
		return docOrderCmp.Compare(navigatorThis, navigatorThat);
	}

	public IList<XPathNavigator> DocOrderDistinct(IList<XPathNavigator> seq)
	{
		if (seq.Count <= 1)
		{
			return seq;
		}
		XmlQueryNodeSequence xmlQueryNodeSequence = (XmlQueryNodeSequence)seq;
		if (xmlQueryNodeSequence == null)
		{
			xmlQueryNodeSequence = new XmlQueryNodeSequence(seq);
		}
		return xmlQueryNodeSequence.DocOrderDistinct(docOrderCmp);
	}

	public string GenerateId(XPathNavigator navigator)
	{
		return "ID" + docOrderCmp.GetDocumentIndex(navigator).ToString(CultureInfo.InvariantCulture) + navigator.UniqueId;
	}

	public bool FindIndex(XPathNavigator context, int indexId, out XmlILIndex index)
	{
		XPathNavigator xPathNavigator = context.Clone();
		xPathNavigator.MoveToRoot();
		if (indexes != null && indexId < indexes.Length)
		{
			ArrayList arrayList = indexes[indexId];
			if (arrayList != null)
			{
				for (int i = 0; i < arrayList.Count; i += 2)
				{
					if (((XPathNavigator)arrayList[i]).IsSamePosition(xPathNavigator))
					{
						index = (XmlILIndex)arrayList[i + 1];
						return true;
					}
				}
			}
		}
		index = new XmlILIndex();
		return false;
	}

	public void AddNewIndex(XPathNavigator context, int indexId, XmlILIndex index)
	{
		XPathNavigator xPathNavigator = context.Clone();
		xPathNavigator.MoveToRoot();
		if (indexes == null)
		{
			indexes = new ArrayList[indexId + 4];
		}
		else if (indexId >= indexes.Length)
		{
			ArrayList[] destinationArray = new ArrayList[indexId + 4];
			Array.Copy(indexes, 0, destinationArray, 0, indexes.Length);
			indexes = destinationArray;
		}
		ArrayList arrayList = indexes[indexId];
		if (arrayList == null)
		{
			arrayList = new ArrayList();
			indexes[indexId] = arrayList;
		}
		arrayList.Add(xPathNavigator);
		arrayList.Add(index);
	}

	public void StartSequenceConstruction(out XmlQueryOutput output)
	{
		stkOutput.Push(this.output);
		output = (this.output = new XmlQueryOutput(this, new XmlCachedSequenceWriter()));
	}

	public IList<XPathItem> EndSequenceConstruction(out XmlQueryOutput output)
	{
		XmlQueryItemSequence resultSequence = ((XmlCachedSequenceWriter)this.output.SequenceWriter).ResultSequence;
		output = (this.output = stkOutput.Pop());
		return resultSequence;
	}

	public void StartRtfConstruction(string baseUri, out XmlQueryOutput output)
	{
		stkOutput.Push(this.output);
		output = (this.output = new XmlQueryOutput(this, new XmlEventCache(baseUri, hasRootNode: true)));
	}

	public XPathNavigator EndRtfConstruction(out XmlQueryOutput output)
	{
		XmlEventCache obj = (XmlEventCache)this.output.Writer;
		output = (this.output = stkOutput.Pop());
		obj.EndEvents();
		return new RtfTreeNavigator(obj, nameTableQuery);
	}

	public XPathNavigator TextRtfConstruction(string text, string baseUri)
	{
		return new RtfTextNavigator(text, baseUri);
	}

	public void SendMessage(string message)
	{
		ctxt.OnXsltMessageEncountered(message);
	}

	public void ThrowException(string text)
	{
		throw new XslTransformException(text);
	}

	internal static XPathNavigator SyncToNavigator(XPathNavigator navigatorThis, XPathNavigator navigatorThat)
	{
		if (navigatorThis == null || !navigatorThis.MoveTo(navigatorThat))
		{
			return navigatorThat.Clone();
		}
		return navigatorThis;
	}

	public static int OnCurrentNodeChanged(XPathNavigator currentNode)
	{
		if (currentNode is IXmlLineInfo xmlLineInfo && (currentNode.NodeType != XPathNodeType.Namespace || !IsInheritedNamespace(currentNode)))
		{
			OnCurrentNodeChanged2(currentNode.BaseURI, xmlLineInfo.LineNumber, xmlLineInfo.LinePosition);
		}
		return 0;
	}

	private static bool IsInheritedNamespace(XPathNavigator node)
	{
		XPathNavigator xPathNavigator = node.Clone();
		if (xPathNavigator.MoveToParent() && xPathNavigator.MoveToFirstNamespace(XPathNamespaceScope.Local))
		{
			do
			{
				if ((object)xPathNavigator.LocalName == node.LocalName)
				{
					return false;
				}
			}
			while (xPathNavigator.MoveToNextNamespace(XPathNamespaceScope.Local));
		}
		return true;
	}

	private static void OnCurrentNodeChanged2(string baseUri, int lineNumber, int linePosition)
	{
	}
}
