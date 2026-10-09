using System.Collections.Generic;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Xml.Schema;
using System.Xml.XPath;
using System.Xml.Xsl.Runtime;

namespace System.Xml.Xsl.IlGen;

internal static class XmlILMethods
{
	public static readonly MethodInfo AncCreate;

	public static readonly MethodInfo AncNext;

	public static readonly MethodInfo AncDOCreate;

	public static readonly MethodInfo AncDONext;

	public static readonly MethodInfo AttrContentCreate;

	public static readonly MethodInfo AttrContentNext;

	public static readonly MethodInfo AttrCreate;

	public static readonly MethodInfo AttrNext;

	public static readonly MethodInfo ContentCreate;

	public static readonly MethodInfo ContentNext;

	public static readonly MethodInfo ContentMergeCreate;

	public static readonly MethodInfo ContentMergeNext;

	public static readonly MethodInfo DescCreate;

	public static readonly MethodInfo DescNext;

	public static readonly MethodInfo DescMergeCreate;

	public static readonly MethodInfo DescMergeNext;

	public static readonly MethodInfo DiffCreate;

	public static readonly MethodInfo DiffNext;

	public static readonly MethodInfo DodMergeCreate;

	public static readonly MethodInfo DodMergeAdd;

	public static readonly MethodInfo DodMergeSeq;

	public static readonly MethodInfo ElemContentCreate;

	public static readonly MethodInfo ElemContentNext;

	public static readonly MethodInfo FollSibCreate;

	public static readonly MethodInfo FollSibNext;

	public static readonly MethodInfo FollSibMergeCreate;

	public static readonly MethodInfo FollSibMergeNext;

	public static readonly MethodInfo IdCreate;

	public static readonly MethodInfo IdNext;

	public static readonly MethodInfo InterCreate;

	public static readonly MethodInfo InterNext;

	public static readonly MethodInfo KindContentCreate;

	public static readonly MethodInfo KindContentNext;

	public static readonly MethodInfo NmspCreate;

	public static readonly MethodInfo NmspNext;

	public static readonly MethodInfo NodeRangeCreate;

	public static readonly MethodInfo NodeRangeNext;

	public static readonly MethodInfo ParentCreate;

	public static readonly MethodInfo ParentNext;

	public static readonly MethodInfo PrecCreate;

	public static readonly MethodInfo PrecNext;

	public static readonly MethodInfo PreSibCreate;

	public static readonly MethodInfo PreSibNext;

	public static readonly MethodInfo PreSibDOCreate;

	public static readonly MethodInfo PreSibDONext;

	public static readonly MethodInfo SortKeyCreate;

	public static readonly MethodInfo SortKeyDateTime;

	public static readonly MethodInfo SortKeyDecimal;

	public static readonly MethodInfo SortKeyDouble;

	public static readonly MethodInfo SortKeyEmpty;

	public static readonly MethodInfo SortKeyFinish;

	public static readonly MethodInfo SortKeyInt;

	public static readonly MethodInfo SortKeyInteger;

	public static readonly MethodInfo SortKeyKeys;

	public static readonly MethodInfo SortKeyString;

	public static readonly MethodInfo UnionCreate;

	public static readonly MethodInfo UnionNext;

	public static readonly MethodInfo XPFollCreate;

	public static readonly MethodInfo XPFollNext;

	public static readonly MethodInfo XPFollMergeCreate;

	public static readonly MethodInfo XPFollMergeNext;

	public static readonly MethodInfo XPPrecCreate;

	public static readonly MethodInfo XPPrecNext;

	public static readonly MethodInfo XPPrecDOCreate;

	public static readonly MethodInfo XPPrecDONext;

	public static readonly MethodInfo XPPrecMergeCreate;

	public static readonly MethodInfo XPPrecMergeNext;

	public static readonly MethodInfo AddNewIndex;

	public static readonly MethodInfo ChangeTypeXsltArg;

	public static readonly MethodInfo ChangeTypeXsltResult;

	public static readonly MethodInfo CompPos;

	public static readonly MethodInfo Context;

	public static readonly MethodInfo CreateCollation;

	public static readonly MethodInfo DocOrder;

	public static readonly MethodInfo EndRtfConstr;

	public static readonly MethodInfo EndSeqConstr;

	public static readonly MethodInfo FindIndex;

	public static readonly MethodInfo GenId;

	public static readonly MethodInfo GetAtomizedName;

	public static readonly MethodInfo GetCollation;

	public static readonly MethodInfo GetEarly;

	public static readonly MethodInfo GetNameFilter;

	public static readonly MethodInfo GetOutput;

	public static readonly MethodInfo GetGlobalValue;

	public static readonly MethodInfo GetTypeFilter;

	public static readonly MethodInfo GlobalComputed;

	public static readonly MethodInfo ItemMatchesCode;

	public static readonly MethodInfo ItemMatchesType;

	public static readonly MethodInfo QNameEqualLit;

	public static readonly MethodInfo QNameEqualNav;

	public static readonly MethodInfo RtfConstr;

	public static readonly MethodInfo SendMessage;

	public static readonly MethodInfo SeqMatchesCode;

	public static readonly MethodInfo SeqMatchesType;

	public static readonly MethodInfo SetGlobalValue;

	public static readonly MethodInfo StartRtfConstr;

	public static readonly MethodInfo StartSeqConstr;

	public static readonly MethodInfo TagAndMappings;

	public static readonly MethodInfo TagAndNamespace;

	public static readonly MethodInfo ThrowException;

	public static readonly MethodInfo XsltLib;

	public static readonly MethodInfo GetDataSource;

	public static readonly MethodInfo GetDefaultDataSource;

	public static readonly MethodInfo GetParam;

	public static readonly MethodInfo InvokeXsltLate;

	public static readonly MethodInfo IndexAdd;

	public static readonly MethodInfo IndexLookup;

	public static readonly MethodInfo ItemIsNode;

	public static readonly MethodInfo Value;

	public static readonly MethodInfo ValueAsAny;

	public static readonly MethodInfo NavClone;

	public static readonly MethodInfo NavLocalName;

	public static readonly MethodInfo NavMoveAttr;

	public static readonly MethodInfo NavMoveId;

	public static readonly MethodInfo NavMoveParent;

	public static readonly MethodInfo NavMoveRoot;

	public static readonly MethodInfo NavMoveTo;

	public static readonly MethodInfo NavNmsp;

	public static readonly MethodInfo NavPrefix;

	public static readonly MethodInfo NavSamePos;

	public static readonly MethodInfo NavType;

	public static readonly MethodInfo StartElemLitName;

	public static readonly MethodInfo StartElemLocName;

	public static readonly MethodInfo EndElemStackName;

	public static readonly MethodInfo StartAttrLitName;

	public static readonly MethodInfo StartAttrLocName;

	public static readonly MethodInfo EndAttr;

	public static readonly MethodInfo Text;

	public static readonly MethodInfo NoEntText;

	public static readonly MethodInfo StartTree;

	public static readonly MethodInfo EndTree;

	public static readonly MethodInfo StartElemLitNameUn;

	public static readonly MethodInfo StartElemLocNameUn;

	public static readonly MethodInfo StartContentUn;

	public static readonly MethodInfo EndElemLitNameUn;

	public static readonly MethodInfo EndElemLocNameUn;

	public static readonly MethodInfo StartAttrLitNameUn;

	public static readonly MethodInfo StartAttrLocNameUn;

	public static readonly MethodInfo EndAttrUn;

	public static readonly MethodInfo NamespaceDeclUn;

	public static readonly MethodInfo TextUn;

	public static readonly MethodInfo NoEntTextUn;

	public static readonly MethodInfo StartRoot;

	public static readonly MethodInfo EndRoot;

	public static readonly MethodInfo StartElemCopyName;

	public static readonly MethodInfo StartElemMapName;

	public static readonly MethodInfo StartElemNmspName;

	public static readonly MethodInfo StartElemQName;

	public static readonly MethodInfo StartAttrCopyName;

	public static readonly MethodInfo StartAttrMapName;

	public static readonly MethodInfo StartAttrNmspName;

	public static readonly MethodInfo StartAttrQName;

	public static readonly MethodInfo NamespaceDecl;

	public static readonly MethodInfo StartComment;

	public static readonly MethodInfo CommentText;

	public static readonly MethodInfo EndComment;

	public static readonly MethodInfo StartPI;

	public static readonly MethodInfo PIText;

	public static readonly MethodInfo EndPI;

	public static readonly MethodInfo WriteItem;

	public static readonly MethodInfo CopyOf;

	public static readonly MethodInfo StartCopy;

	public static readonly MethodInfo EndCopy;

	public static readonly MethodInfo DecAdd;

	public static readonly MethodInfo DecCmp;

	public static readonly MethodInfo DecEq;

	public static readonly MethodInfo DecSub;

	public static readonly MethodInfo DecMul;

	public static readonly MethodInfo DecDiv;

	public static readonly MethodInfo DecRem;

	public static readonly MethodInfo DecNeg;

	public static readonly MethodInfo QNameEq;

	public static readonly MethodInfo StrEq;

	public static readonly MethodInfo StrCat2;

	public static readonly MethodInfo StrCat3;

	public static readonly MethodInfo StrCat4;

	public static readonly MethodInfo StrCmp;

	public static readonly MethodInfo StrLen;

	public static readonly MethodInfo DblToDec;

	public static readonly MethodInfo DblToInt;

	public static readonly MethodInfo DblToLng;

	public static readonly MethodInfo DblToStr;

	public static readonly MethodInfo DecToDbl;

	public static readonly MethodInfo DTToStr;

	public static readonly MethodInfo IntToDbl;

	public static readonly MethodInfo LngToDbl;

	public static readonly MethodInfo StrToDbl;

	public static readonly MethodInfo StrToDT;

	public static readonly MethodInfo ItemToBool;

	public static readonly MethodInfo ItemToDbl;

	public static readonly MethodInfo ItemToStr;

	public static readonly MethodInfo ItemToNode;

	public static readonly MethodInfo ItemToNodes;

	public static readonly MethodInfo ItemsToBool;

	public static readonly MethodInfo ItemsToDbl;

	public static readonly MethodInfo ItemsToNode;

	public static readonly MethodInfo ItemsToNodes;

	public static readonly MethodInfo ItemsToStr;

	public static readonly MethodInfo StrCatCat;

	public static readonly MethodInfo StrCatClear;

	public static readonly MethodInfo StrCatResult;

	public static readonly MethodInfo StrCatDelim;

	public static readonly MethodInfo NavsToItems;

	public static readonly MethodInfo ItemsToNavs;

	public static readonly MethodInfo SetDod;

	public static readonly MethodInfo GetTypeFromHandle;

	public static readonly MethodInfo InitializeArray;

	public static readonly Dictionary<Type, XmlILStorageMethods> StorageMethods;

	static XmlILMethods()
	{
		AncCreate = GetMethod(typeof(AncestorIterator), "Create");
		AncNext = GetMethod(typeof(AncestorIterator), "MoveNext");
		AncDOCreate = GetMethod(typeof(AncestorDocOrderIterator), "Create");
		AncDONext = GetMethod(typeof(AncestorDocOrderIterator), "MoveNext");
		AttrContentCreate = GetMethod(typeof(AttributeContentIterator), "Create");
		AttrContentNext = GetMethod(typeof(AttributeContentIterator), "MoveNext");
		AttrCreate = GetMethod(typeof(AttributeIterator), "Create");
		AttrNext = GetMethod(typeof(AttributeIterator), "MoveNext");
		ContentCreate = GetMethod(typeof(ContentIterator), "Create");
		ContentNext = GetMethod(typeof(ContentIterator), "MoveNext");
		ContentMergeCreate = GetMethod(typeof(ContentMergeIterator), "Create");
		ContentMergeNext = GetMethod(typeof(ContentMergeIterator), "MoveNext");
		DescCreate = GetMethod(typeof(DescendantIterator), "Create");
		DescNext = GetMethod(typeof(DescendantIterator), "MoveNext");
		DescMergeCreate = GetMethod(typeof(DescendantMergeIterator), "Create");
		DescMergeNext = GetMethod(typeof(DescendantMergeIterator), "MoveNext");
		DiffCreate = GetMethod(typeof(DifferenceIterator), "Create");
		DiffNext = GetMethod(typeof(DifferenceIterator), "MoveNext");
		DodMergeCreate = GetMethod(typeof(DodSequenceMerge), "Create");
		DodMergeAdd = GetMethod(typeof(DodSequenceMerge), "AddSequence");
		DodMergeSeq = GetMethod(typeof(DodSequenceMerge), "MergeSequences");
		ElemContentCreate = GetMethod(typeof(ElementContentIterator), "Create");
		ElemContentNext = GetMethod(typeof(ElementContentIterator), "MoveNext");
		FollSibCreate = GetMethod(typeof(FollowingSiblingIterator), "Create");
		FollSibNext = GetMethod(typeof(FollowingSiblingIterator), "MoveNext");
		FollSibMergeCreate = GetMethod(typeof(FollowingSiblingMergeIterator), "Create");
		FollSibMergeNext = GetMethod(typeof(FollowingSiblingMergeIterator), "MoveNext");
		IdCreate = GetMethod(typeof(IdIterator), "Create");
		IdNext = GetMethod(typeof(IdIterator), "MoveNext");
		InterCreate = GetMethod(typeof(IntersectIterator), "Create");
		InterNext = GetMethod(typeof(IntersectIterator), "MoveNext");
		KindContentCreate = GetMethod(typeof(NodeKindContentIterator), "Create");
		KindContentNext = GetMethod(typeof(NodeKindContentIterator), "MoveNext");
		NmspCreate = GetMethod(typeof(NamespaceIterator), "Create");
		NmspNext = GetMethod(typeof(NamespaceIterator), "MoveNext");
		NodeRangeCreate = GetMethod(typeof(NodeRangeIterator), "Create");
		NodeRangeNext = GetMethod(typeof(NodeRangeIterator), "MoveNext");
		ParentCreate = GetMethod(typeof(ParentIterator), "Create");
		ParentNext = GetMethod(typeof(ParentIterator), "MoveNext");
		PrecCreate = GetMethod(typeof(PrecedingIterator), "Create");
		PrecNext = GetMethod(typeof(PrecedingIterator), "MoveNext");
		PreSibCreate = GetMethod(typeof(PrecedingSiblingIterator), "Create");
		PreSibNext = GetMethod(typeof(PrecedingSiblingIterator), "MoveNext");
		PreSibDOCreate = GetMethod(typeof(PrecedingSiblingDocOrderIterator), "Create");
		PreSibDONext = GetMethod(typeof(PrecedingSiblingDocOrderIterator), "MoveNext");
		SortKeyCreate = GetMethod(typeof(XmlSortKeyAccumulator), "Create");
		SortKeyDateTime = GetMethod(typeof(XmlSortKeyAccumulator), "AddDateTimeSortKey");
		SortKeyDecimal = GetMethod(typeof(XmlSortKeyAccumulator), "AddDecimalSortKey");
		SortKeyDouble = GetMethod(typeof(XmlSortKeyAccumulator), "AddDoubleSortKey");
		SortKeyEmpty = GetMethod(typeof(XmlSortKeyAccumulator), "AddEmptySortKey");
		SortKeyFinish = GetMethod(typeof(XmlSortKeyAccumulator), "FinishSortKeys");
		SortKeyInt = GetMethod(typeof(XmlSortKeyAccumulator), "AddIntSortKey");
		SortKeyInteger = GetMethod(typeof(XmlSortKeyAccumulator), "AddIntegerSortKey");
		SortKeyKeys = GetMethod(typeof(XmlSortKeyAccumulator), "get_Keys");
		SortKeyString = GetMethod(typeof(XmlSortKeyAccumulator), "AddStringSortKey");
		UnionCreate = GetMethod(typeof(UnionIterator), "Create");
		UnionNext = GetMethod(typeof(UnionIterator), "MoveNext");
		XPFollCreate = GetMethod(typeof(XPathFollowingIterator), "Create");
		XPFollNext = GetMethod(typeof(XPathFollowingIterator), "MoveNext");
		XPFollMergeCreate = GetMethod(typeof(XPathFollowingMergeIterator), "Create");
		XPFollMergeNext = GetMethod(typeof(XPathFollowingMergeIterator), "MoveNext");
		XPPrecCreate = GetMethod(typeof(XPathPrecedingIterator), "Create");
		XPPrecNext = GetMethod(typeof(XPathPrecedingIterator), "MoveNext");
		XPPrecDOCreate = GetMethod(typeof(XPathPrecedingDocOrderIterator), "Create");
		XPPrecDONext = GetMethod(typeof(XPathPrecedingDocOrderIterator), "MoveNext");
		XPPrecMergeCreate = GetMethod(typeof(XPathPrecedingMergeIterator), "Create");
		XPPrecMergeNext = GetMethod(typeof(XPathPrecedingMergeIterator), "MoveNext");
		AddNewIndex = GetMethod(typeof(XmlQueryRuntime), "AddNewIndex");
		ChangeTypeXsltArg = GetMethod(typeof(XmlQueryRuntime), "ChangeTypeXsltArgument", typeof(int), typeof(object), typeof(Type));
		ChangeTypeXsltResult = GetMethod(typeof(XmlQueryRuntime), "ChangeTypeXsltResult");
		CompPos = GetMethod(typeof(XmlQueryRuntime), "ComparePosition");
		Context = GetMethod(typeof(XmlQueryRuntime), "get_ExternalContext");
		CreateCollation = GetMethod(typeof(XmlQueryRuntime), "CreateCollation");
		DocOrder = GetMethod(typeof(XmlQueryRuntime), "DocOrderDistinct");
		EndRtfConstr = GetMethod(typeof(XmlQueryRuntime), "EndRtfConstruction");
		EndSeqConstr = GetMethod(typeof(XmlQueryRuntime), "EndSequenceConstruction");
		FindIndex = GetMethod(typeof(XmlQueryRuntime), "FindIndex");
		GenId = GetMethod(typeof(XmlQueryRuntime), "GenerateId");
		GetAtomizedName = GetMethod(typeof(XmlQueryRuntime), "GetAtomizedName");
		GetCollation = GetMethod(typeof(XmlQueryRuntime), "GetCollation");
		GetEarly = GetMethod(typeof(XmlQueryRuntime), "GetEarlyBoundObject");
		GetNameFilter = GetMethod(typeof(XmlQueryRuntime), "GetNameFilter");
		GetOutput = GetMethod(typeof(XmlQueryRuntime), "get_Output");
		GetGlobalValue = GetMethod(typeof(XmlQueryRuntime), "GetGlobalValue");
		GetTypeFilter = GetMethod(typeof(XmlQueryRuntime), "GetTypeFilter");
		GlobalComputed = GetMethod(typeof(XmlQueryRuntime), "IsGlobalComputed");
		ItemMatchesCode = GetMethod(typeof(XmlQueryRuntime), "MatchesXmlType", typeof(XPathItem), typeof(XmlTypeCode));
		ItemMatchesType = GetMethod(typeof(XmlQueryRuntime), "MatchesXmlType", typeof(XPathItem), typeof(int));
		QNameEqualLit = GetMethod(typeof(XmlQueryRuntime), "IsQNameEqual", typeof(XPathNavigator), typeof(int), typeof(int));
		QNameEqualNav = GetMethod(typeof(XmlQueryRuntime), "IsQNameEqual", typeof(XPathNavigator), typeof(XPathNavigator));
		RtfConstr = GetMethod(typeof(XmlQueryRuntime), "TextRtfConstruction");
		SendMessage = GetMethod(typeof(XmlQueryRuntime), "SendMessage");
		SeqMatchesCode = GetMethod(typeof(XmlQueryRuntime), "MatchesXmlType", typeof(IList<XPathItem>), typeof(XmlTypeCode));
		SeqMatchesType = GetMethod(typeof(XmlQueryRuntime), "MatchesXmlType", typeof(IList<XPathItem>), typeof(int));
		SetGlobalValue = GetMethod(typeof(XmlQueryRuntime), "SetGlobalValue");
		StartRtfConstr = GetMethod(typeof(XmlQueryRuntime), "StartRtfConstruction");
		StartSeqConstr = GetMethod(typeof(XmlQueryRuntime), "StartSequenceConstruction");
		TagAndMappings = GetMethod(typeof(XmlQueryRuntime), "ParseTagName", typeof(string), typeof(int));
		TagAndNamespace = GetMethod(typeof(XmlQueryRuntime), "ParseTagName", typeof(string), typeof(string));
		ThrowException = GetMethod(typeof(XmlQueryRuntime), "ThrowException");
		XsltLib = GetMethod(typeof(XmlQueryRuntime), "get_XsltFunctions");
		GetDataSource = GetMethod(typeof(XmlQueryContext), "GetDataSource");
		GetDefaultDataSource = GetMethod(typeof(XmlQueryContext), "get_DefaultDataSource");
		GetParam = GetMethod(typeof(XmlQueryContext), "GetParameter");
		InvokeXsltLate = GetMethod(typeof(XmlQueryContext), "InvokeXsltLateBoundFunction");
		IndexAdd = GetMethod(typeof(XmlILIndex), "Add");
		IndexLookup = GetMethod(typeof(XmlILIndex), "Lookup");
		ItemIsNode = GetMethod(typeof(XPathItem), "get_IsNode");
		Value = GetMethod(typeof(XPathItem), "get_Value");
		ValueAsAny = GetMethod(typeof(XPathItem), "ValueAs", typeof(Type), typeof(IXmlNamespaceResolver));
		NavClone = GetMethod(typeof(XPathNavigator), "Clone");
		NavLocalName = GetMethod(typeof(XPathNavigator), "get_LocalName");
		NavMoveAttr = GetMethod(typeof(XPathNavigator), "MoveToAttribute", typeof(string), typeof(string));
		NavMoveId = GetMethod(typeof(XPathNavigator), "MoveToId");
		NavMoveParent = GetMethod(typeof(XPathNavigator), "MoveToParent");
		NavMoveRoot = GetMethod(typeof(XPathNavigator), "MoveToRoot");
		NavMoveTo = GetMethod(typeof(XPathNavigator), "MoveTo");
		NavNmsp = GetMethod(typeof(XPathNavigator), "get_NamespaceURI");
		NavPrefix = GetMethod(typeof(XPathNavigator), "get_Prefix");
		NavSamePos = GetMethod(typeof(XPathNavigator), "IsSamePosition");
		NavType = GetMethod(typeof(XPathNavigator), "get_NodeType");
		StartElemLitName = GetMethod(typeof(XmlQueryOutput), "WriteStartElement", typeof(string), typeof(string), typeof(string));
		StartElemLocName = GetMethod(typeof(XmlQueryOutput), "WriteStartElementLocalName", typeof(string));
		EndElemStackName = GetMethod(typeof(XmlQueryOutput), "WriteEndElement");
		StartAttrLitName = GetMethod(typeof(XmlQueryOutput), "WriteStartAttribute", typeof(string), typeof(string), typeof(string));
		StartAttrLocName = GetMethod(typeof(XmlQueryOutput), "WriteStartAttributeLocalName", typeof(string));
		EndAttr = GetMethod(typeof(XmlQueryOutput), "WriteEndAttribute");
		Text = GetMethod(typeof(XmlQueryOutput), "WriteString");
		NoEntText = GetMethod(typeof(XmlQueryOutput), "WriteRaw", typeof(string));
		StartTree = GetMethod(typeof(XmlQueryOutput), "StartTree");
		EndTree = GetMethod(typeof(XmlQueryOutput), "EndTree");
		StartElemLitNameUn = GetMethod(typeof(XmlQueryOutput), "WriteStartElementUnchecked", typeof(string), typeof(string), typeof(string));
		StartElemLocNameUn = GetMethod(typeof(XmlQueryOutput), "WriteStartElementUnchecked", typeof(string));
		StartContentUn = GetMethod(typeof(XmlQueryOutput), "StartElementContentUnchecked");
		EndElemLitNameUn = GetMethod(typeof(XmlQueryOutput), "WriteEndElementUnchecked", typeof(string), typeof(string), typeof(string));
		EndElemLocNameUn = GetMethod(typeof(XmlQueryOutput), "WriteEndElementUnchecked", typeof(string));
		StartAttrLitNameUn = GetMethod(typeof(XmlQueryOutput), "WriteStartAttributeUnchecked", typeof(string), typeof(string), typeof(string));
		StartAttrLocNameUn = GetMethod(typeof(XmlQueryOutput), "WriteStartAttributeUnchecked", typeof(string));
		EndAttrUn = GetMethod(typeof(XmlQueryOutput), "WriteEndAttributeUnchecked");
		NamespaceDeclUn = GetMethod(typeof(XmlQueryOutput), "WriteNamespaceDeclarationUnchecked");
		TextUn = GetMethod(typeof(XmlQueryOutput), "WriteStringUnchecked");
		NoEntTextUn = GetMethod(typeof(XmlQueryOutput), "WriteRawUnchecked");
		StartRoot = GetMethod(typeof(XmlQueryOutput), "WriteStartRoot");
		EndRoot = GetMethod(typeof(XmlQueryOutput), "WriteEndRoot");
		StartElemCopyName = GetMethod(typeof(XmlQueryOutput), "WriteStartElementComputed", typeof(XPathNavigator));
		StartElemMapName = GetMethod(typeof(XmlQueryOutput), "WriteStartElementComputed", typeof(string), typeof(int));
		StartElemNmspName = GetMethod(typeof(XmlQueryOutput), "WriteStartElementComputed", typeof(string), typeof(string));
		StartElemQName = GetMethod(typeof(XmlQueryOutput), "WriteStartElementComputed", typeof(XmlQualifiedName));
		StartAttrCopyName = GetMethod(typeof(XmlQueryOutput), "WriteStartAttributeComputed", typeof(XPathNavigator));
		StartAttrMapName = GetMethod(typeof(XmlQueryOutput), "WriteStartAttributeComputed", typeof(string), typeof(int));
		StartAttrNmspName = GetMethod(typeof(XmlQueryOutput), "WriteStartAttributeComputed", typeof(string), typeof(string));
		StartAttrQName = GetMethod(typeof(XmlQueryOutput), "WriteStartAttributeComputed", typeof(XmlQualifiedName));
		NamespaceDecl = GetMethod(typeof(XmlQueryOutput), "WriteNamespaceDeclaration");
		StartComment = GetMethod(typeof(XmlQueryOutput), "WriteStartComment");
		CommentText = GetMethod(typeof(XmlQueryOutput), "WriteCommentString");
		EndComment = GetMethod(typeof(XmlQueryOutput), "WriteEndComment");
		StartPI = GetMethod(typeof(XmlQueryOutput), "WriteStartProcessingInstruction");
		PIText = GetMethod(typeof(XmlQueryOutput), "WriteProcessingInstructionString");
		EndPI = GetMethod(typeof(XmlQueryOutput), "WriteEndProcessingInstruction");
		WriteItem = GetMethod(typeof(XmlQueryOutput), "WriteItem");
		CopyOf = GetMethod(typeof(XmlQueryOutput), "XsltCopyOf");
		StartCopy = GetMethod(typeof(XmlQueryOutput), "StartCopy");
		EndCopy = GetMethod(typeof(XmlQueryOutput), "EndCopy");
		DecAdd = GetMethod(typeof(decimal), "Add");
		DecCmp = GetMethod(typeof(decimal), "Compare", typeof(decimal), typeof(decimal));
		DecEq = GetMethod(typeof(decimal), "Equals", typeof(decimal), typeof(decimal));
		DecSub = GetMethod(typeof(decimal), "Subtract");
		DecMul = GetMethod(typeof(decimal), "Multiply");
		DecDiv = GetMethod(typeof(decimal), "Divide");
		DecRem = GetMethod(typeof(decimal), "Remainder");
		DecNeg = GetMethod(typeof(decimal), "Negate");
		QNameEq = GetMethod(typeof(XmlQualifiedName), "Equals");
		StrEq = GetMethod(typeof(string), "Equals", typeof(string), typeof(string));
		StrCat2 = GetMethod(typeof(string), "Concat", typeof(string), typeof(string));
		StrCat3 = GetMethod(typeof(string), "Concat", typeof(string), typeof(string), typeof(string));
		StrCat4 = GetMethod(typeof(string), "Concat", typeof(string), typeof(string), typeof(string), typeof(string));
		StrCmp = GetMethod(typeof(string), "CompareOrdinal", typeof(string), typeof(string));
		StrLen = GetMethod(typeof(string), "get_Length");
		DblToDec = GetMethod(typeof(XsltConvert), "ToDecimal", typeof(double));
		DblToInt = GetMethod(typeof(XsltConvert), "ToInt", typeof(double));
		DblToLng = GetMethod(typeof(XsltConvert), "ToLong", typeof(double));
		DblToStr = GetMethod(typeof(XsltConvert), "ToString", typeof(double));
		DecToDbl = GetMethod(typeof(XsltConvert), "ToDouble", typeof(decimal));
		DTToStr = GetMethod(typeof(XsltConvert), "ToString", typeof(DateTime));
		IntToDbl = GetMethod(typeof(XsltConvert), "ToDouble", typeof(int));
		LngToDbl = GetMethod(typeof(XsltConvert), "ToDouble", typeof(long));
		StrToDbl = GetMethod(typeof(XsltConvert), "ToDouble", typeof(string));
		StrToDT = GetMethod(typeof(XsltConvert), "ToDateTime", typeof(string));
		ItemToBool = GetMethod(typeof(XsltConvert), "ToBoolean", typeof(XPathItem));
		ItemToDbl = GetMethod(typeof(XsltConvert), "ToDouble", typeof(XPathItem));
		ItemToStr = GetMethod(typeof(XsltConvert), "ToString", typeof(XPathItem));
		ItemToNode = GetMethod(typeof(XsltConvert), "ToNode", typeof(XPathItem));
		ItemToNodes = GetMethod(typeof(XsltConvert), "ToNodeSet", typeof(XPathItem));
		ItemsToBool = GetMethod(typeof(XsltConvert), "ToBoolean", typeof(IList<XPathItem>));
		ItemsToDbl = GetMethod(typeof(XsltConvert), "ToDouble", typeof(IList<XPathItem>));
		ItemsToNode = GetMethod(typeof(XsltConvert), "ToNode", typeof(IList<XPathItem>));
		ItemsToNodes = GetMethod(typeof(XsltConvert), "ToNodeSet", typeof(IList<XPathItem>));
		ItemsToStr = GetMethod(typeof(XsltConvert), "ToString", typeof(IList<XPathItem>));
		StrCatCat = GetMethod(typeof(StringConcat), "Concat");
		StrCatClear = GetMethod(typeof(StringConcat), "Clear");
		StrCatResult = GetMethod(typeof(StringConcat), "GetResult");
		StrCatDelim = GetMethod(typeof(StringConcat), "set_Delimiter");
		NavsToItems = GetMethod(typeof(XmlILStorageConverter), "NavigatorsToItems");
		ItemsToNavs = GetMethod(typeof(XmlILStorageConverter), "ItemsToNavigators");
		SetDod = GetMethod(typeof(XmlQueryNodeSequence), "set_IsDocOrderDistinct");
		GetTypeFromHandle = GetMethod(typeof(Type), "GetTypeFromHandle");
		InitializeArray = GetMethod(typeof(RuntimeHelpers), "InitializeArray");
		StorageMethods = new Dictionary<Type, XmlILStorageMethods>();
		StorageMethods[typeof(string)] = new XmlILStorageMethods(typeof(string));
		StorageMethods[typeof(bool)] = new XmlILStorageMethods(typeof(bool));
		StorageMethods[typeof(int)] = new XmlILStorageMethods(typeof(int));
		StorageMethods[typeof(long)] = new XmlILStorageMethods(typeof(long));
		StorageMethods[typeof(decimal)] = new XmlILStorageMethods(typeof(decimal));
		StorageMethods[typeof(double)] = new XmlILStorageMethods(typeof(double));
		StorageMethods[typeof(float)] = new XmlILStorageMethods(typeof(float));
		StorageMethods[typeof(DateTime)] = new XmlILStorageMethods(typeof(DateTime));
		StorageMethods[typeof(byte[])] = new XmlILStorageMethods(typeof(byte[]));
		StorageMethods[typeof(XmlQualifiedName)] = new XmlILStorageMethods(typeof(XmlQualifiedName));
		StorageMethods[typeof(TimeSpan)] = new XmlILStorageMethods(typeof(TimeSpan));
		StorageMethods[typeof(XPathItem)] = new XmlILStorageMethods(typeof(XPathItem));
		StorageMethods[typeof(XPathNavigator)] = new XmlILStorageMethods(typeof(XPathNavigator));
	}

	public static MethodInfo GetMethod(Type className, string methName)
	{
		return className.GetMethod(methName);
	}

	public static MethodInfo GetMethod(Type className, string methName, params Type[] args)
	{
		return className.GetMethod(methName, args);
	}
}
