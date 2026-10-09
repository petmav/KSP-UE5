using System.Collections.Generic;
using System.Xml.Xsl.Runtime;

namespace System.Xml.Xsl.Xslt;

internal class XsltInput : IErrorHelper
{
	public struct DelayedQName
	{
		private string prefix;

		private string localName;

		public DelayedQName(ref Record rec)
		{
			prefix = rec.prefix;
			localName = rec.localName;
		}

		public static implicit operator string(DelayedQName qn)
		{
			if (qn.prefix.Length != 0)
			{
				return qn.prefix + ":" + qn.localName;
			}
			return qn.localName;
		}
	}

	public struct XsltAttribute
	{
		public string name;

		public int flags;

		public XsltAttribute(string name, int flags)
		{
			this.name = name;
			this.flags = flags;
		}
	}

	internal class ContextInfo
	{
		internal class EmptyElementEndTag : ISourceLineInfo
		{
			private ISourceLineInfo elementTagLi;

			public string Uri => elementTagLi.Uri;

			public bool IsNoSource => elementTagLi.IsNoSource;

			public Location Start => new Location(elementTagLi.End.Line, elementTagLi.End.Pos - 2);

			public Location End => elementTagLi.End;

			public EmptyElementEndTag(ISourceLineInfo elementTagLi)
			{
				this.elementTagLi = elementTagLi;
			}
		}

		public NsDecl nsList;

		public ISourceLineInfo lineInfo;

		public ISourceLineInfo elemNameLi;

		public ISourceLineInfo endTagLi;

		private int elemNameLength;

		internal ContextInfo(ISourceLineInfo lineinfo)
		{
			elemNameLi = lineinfo;
			endTagLi = lineinfo;
			lineInfo = lineinfo;
		}

		public ContextInfo(XsltInput input)
		{
			elemNameLength = input.QualifiedName.Length;
		}

		public void AddNamespace(string prefix, string nsUri)
		{
			nsList = new NsDecl(nsList, prefix, nsUri);
		}

		public void SaveExtendedLineInfo(XsltInput input)
		{
			if (lineInfo.Start.Line == 0)
			{
				elemNameLi = (endTagLi = null);
				return;
			}
			elemNameLi = new SourceLineInfo(lineInfo.Uri, lineInfo.Start.Line, lineInfo.Start.Pos + 1, lineInfo.Start.Line, lineInfo.Start.Pos + 1 + elemNameLength);
			if (!input.IsEmptyElement)
			{
				endTagLi = input.BuildLineInfo();
			}
			else
			{
				endTagLi = new EmptyElementEndTag(lineInfo);
			}
		}
	}

	internal struct Record
	{
		public string localName;

		public string nsUri;

		public string prefix;

		public string value;

		public string baseUri;

		public Location start;

		public Location valueStart;

		public Location end;

		public string QualifiedName
		{
			get
			{
				if (prefix.Length != 0)
				{
					return prefix + ":" + localName;
				}
				return localName;
			}
		}
	}

	private const int InitRecordsSize = 22;

	private XmlReader reader;

	private IXmlLineInfo readerLineInfo;

	private bool topLevelReader;

	private CompilerScopeManager<VarPar> scopeManager;

	private KeywordsTable atoms;

	private Compiler compiler;

	private bool reatomize;

	private XmlNodeType nodeType;

	private Record[] records = new Record[22];

	private int currentRecord;

	private bool isEmptyElement;

	private int lastTextNode;

	private int numAttributes;

	private ContextInfo ctxInfo;

	private bool attributesRead;

	private StringConcat strConcat;

	private XsltAttribute[] attributes;

	private int[] xsltAttributeNumber = new int[21];

	private static XsltAttribute[] noAttributes = new XsltAttribute[0];

	public XmlNodeType NodeType
	{
		get
		{
			if (nodeType != XmlNodeType.Element || 0 >= currentRecord)
			{
				return nodeType;
			}
			return XmlNodeType.Attribute;
		}
	}

	public string LocalName => records[currentRecord].localName;

	public string NamespaceUri => records[currentRecord].nsUri;

	public string Prefix => records[currentRecord].prefix;

	public string Value => records[currentRecord].value;

	public string BaseUri => records[currentRecord].baseUri;

	public string QualifiedName => records[currentRecord].QualifiedName;

	public bool IsEmptyElement => isEmptyElement;

	public string Uri => records[currentRecord].baseUri;

	public Location Start => records[currentRecord].start;

	public Location End => records[currentRecord].end;

	public DelayedQName ElementName => new DelayedQName(ref records[0]);

	public bool CanHaveApplyImports
	{
		get
		{
			return scopeManager.CanHaveApplyImports;
		}
		set
		{
			scopeManager.CanHaveApplyImports = value;
		}
	}

	public bool ForwardCompatibility => scopeManager.ForwardCompatibility;

	public bool BackwardCompatibility => scopeManager.BackwardCompatibility;

	public XslVersion XslVersion
	{
		get
		{
			if (!scopeManager.ForwardCompatibility)
			{
				return XslVersion.Version10;
			}
			return XslVersion.ForwardsCompatible;
		}
	}

	public XsltInput(XmlReader reader, Compiler compiler, KeywordsTable atoms)
	{
		EnsureExpandEntities(reader);
		IXmlLineInfo xmlLineInfo = reader as IXmlLineInfo;
		this.atoms = atoms;
		this.reader = reader;
		reatomize = reader.NameTable != atoms.NameTable;
		readerLineInfo = ((xmlLineInfo != null && xmlLineInfo.HasLineInfo()) ? xmlLineInfo : null);
		topLevelReader = reader.ReadState == ReadState.Initial;
		scopeManager = new CompilerScopeManager<VarPar>(atoms);
		this.compiler = compiler;
		nodeType = XmlNodeType.Document;
	}

	private static void EnsureExpandEntities(XmlReader reader)
	{
		if (reader is XmlTextReader { EntityHandling: not EntityHandling.ExpandEntities } xmlTextReader)
		{
			xmlTextReader.EntityHandling = EntityHandling.ExpandEntities;
		}
	}

	private void ExtendRecordBuffer(int position)
	{
		if (records.Length <= position)
		{
			int num = records.Length * 2;
			if (num <= position)
			{
				num = position + 1;
			}
			Record[] destinationArray = new Record[num];
			Array.Copy(records, destinationArray, records.Length);
			records = destinationArray;
		}
	}

	public bool FindStylesheetElement()
	{
		if (!topLevelReader && reader.ReadState != ReadState.Interactive)
		{
			return false;
		}
		IDictionary<string, string> dictionary = null;
		if (reader.ReadState == ReadState.Interactive && reader is IXmlNamespaceResolver xmlNamespaceResolver)
		{
			dictionary = xmlNamespaceResolver.GetNamespacesInScope(XmlNamespaceScope.ExcludeXml);
		}
		while (MoveToNextSibling() && nodeType == XmlNodeType.Whitespace)
		{
		}
		if (nodeType == XmlNodeType.Element)
		{
			if (dictionary != null)
			{
				foreach (KeyValuePair<string, string> item in dictionary)
				{
					if (scopeManager.LookupNamespace(item.Key) == null)
					{
						string nsUri = atoms.NameTable.Add(item.Value);
						scopeManager.AddNsDeclaration(item.Key, nsUri);
						ctxInfo.AddNamespace(item.Key, nsUri);
					}
				}
			}
			return true;
		}
		return false;
	}

	public void Finish()
	{
		if (topLevelReader)
		{
			while (reader.ReadState == ReadState.Interactive)
			{
				reader.Skip();
			}
		}
	}

	private void FillupRecord(ref Record rec)
	{
		rec.localName = reader.LocalName;
		rec.nsUri = reader.NamespaceURI;
		rec.prefix = reader.Prefix;
		rec.value = reader.Value;
		rec.baseUri = reader.BaseURI;
		if (reatomize)
		{
			rec.localName = atoms.NameTable.Add(rec.localName);
			rec.nsUri = atoms.NameTable.Add(rec.nsUri);
			rec.prefix = atoms.NameTable.Add(rec.prefix);
		}
		if (readerLineInfo != null)
		{
			rec.start = new Location(readerLineInfo.LineNumber, readerLineInfo.LinePosition - PositionAdjustment(reader.NodeType));
		}
	}

	private void SetRecordEnd(ref Record rec)
	{
		if (readerLineInfo != null)
		{
			rec.end = new Location(readerLineInfo.LineNumber, readerLineInfo.LinePosition - PositionAdjustment(reader.NodeType));
			if (reader.BaseURI != rec.baseUri || rec.end.LessOrEqual(rec.start))
			{
				rec.end = new Location(rec.start.Line, int.MaxValue);
			}
		}
	}

	private void FillupTextRecord(ref Record rec)
	{
		rec.localName = string.Empty;
		rec.nsUri = string.Empty;
		rec.prefix = string.Empty;
		rec.value = reader.Value;
		rec.baseUri = reader.BaseURI;
		if (readerLineInfo == null)
		{
			return;
		}
		bool flag = reader.NodeType == XmlNodeType.CDATA;
		int num = readerLineInfo.LineNumber;
		int num2 = readerLineInfo.LinePosition;
		rec.start = new Location(num, num2 - (flag ? 9 : 0));
		char c = ' ';
		string value = rec.value;
		char c2;
		for (int i = 0; i < value.Length; c = c2, i++)
		{
			c2 = value[i];
			if (c2 != '\n')
			{
				if (c2 != '\r')
				{
					num2++;
					continue;
				}
			}
			else if (c == '\r')
			{
				continue;
			}
			num++;
			num2 = 1;
		}
		rec.end = new Location(num, num2 + (flag ? 3 : 0));
	}

	private void FillupCharacterEntityRecord(ref Record rec)
	{
		_ = reader.LocalName;
		rec.localName = string.Empty;
		rec.nsUri = string.Empty;
		rec.prefix = string.Empty;
		rec.baseUri = reader.BaseURI;
		if (readerLineInfo != null)
		{
			rec.start = new Location(readerLineInfo.LineNumber, readerLineInfo.LinePosition - 1);
		}
		reader.ResolveEntity();
		reader.Read();
		rec.value = reader.Value;
		reader.Read();
		if (readerLineInfo != null)
		{
			_ = readerLineInfo.LineNumber;
			_ = readerLineInfo.LinePosition;
			rec.end = new Location(readerLineInfo.LineNumber, readerLineInfo.LinePosition + 1);
		}
	}

	private bool ReadAttribute(ref Record rec)
	{
		FillupRecord(ref rec);
		if (Ref.Equal(rec.prefix, atoms.Xmlns))
		{
			string nsUri = atoms.NameTable.Add(reader.Value);
			if (!Ref.Equal(rec.localName, atoms.Xml))
			{
				scopeManager.AddNsDeclaration(rec.localName, nsUri);
				ctxInfo.AddNamespace(rec.localName, nsUri);
			}
			return false;
		}
		if (rec.prefix.Length == 0 && Ref.Equal(rec.localName, atoms.Xmlns))
		{
			string nsUri2 = atoms.NameTable.Add(reader.Value);
			scopeManager.AddNsDeclaration(string.Empty, nsUri2);
			ctxInfo.AddNamespace(string.Empty, nsUri2);
			return false;
		}
		if (!reader.ReadAttributeValue())
		{
			rec.value = string.Empty;
			SetRecordEnd(ref rec);
			return true;
		}
		if (readerLineInfo != null)
		{
			int num = ((reader.NodeType == XmlNodeType.EntityReference) ? (-2) : (-1));
			rec.valueStart = new Location(readerLineInfo.LineNumber, readerLineInfo.LinePosition + num);
			if (reader.BaseURI != rec.baseUri || rec.valueStart.LessOrEqual(rec.start))
			{
				int num2 = ((rec.prefix.Length != 0) ? (rec.prefix.Length + 1) : 0) + rec.localName.Length;
				rec.end = new Location(rec.start.Line, rec.start.Pos + num2 + 1);
			}
		}
		string text = string.Empty;
		strConcat.Clear();
		do
		{
			switch (reader.NodeType)
			{
			case XmlNodeType.EntityReference:
				reader.ResolveEntity();
				break;
			default:
				text = reader.Value;
				strConcat.Concat(text);
				break;
			case XmlNodeType.EndEntity:
				break;
			}
		}
		while (reader.ReadAttributeValue());
		rec.value = strConcat.GetResult();
		if (readerLineInfo != null)
		{
			int num3 = ((reader.NodeType == XmlNodeType.EndEntity) ? 1 : text.Length) + 1;
			rec.end = new Location(readerLineInfo.LineNumber, readerLineInfo.LinePosition + num3);
			if (reader.BaseURI != rec.baseUri || rec.end.LessOrEqual(rec.valueStart))
			{
				rec.end = new Location(rec.start.Line, int.MaxValue);
			}
		}
		return true;
	}

	public bool MoveToFirstChild()
	{
		if (IsEmptyElement)
		{
			return false;
		}
		return ReadNextSibling();
	}

	public bool MoveToNextSibling()
	{
		if (nodeType == XmlNodeType.Element || nodeType == XmlNodeType.EndElement)
		{
			scopeManager.ExitScope();
		}
		return ReadNextSibling();
	}

	public void SkipNode()
	{
		if (nodeType == XmlNodeType.Element && MoveToFirstChild())
		{
			do
			{
				SkipNode();
			}
			while (MoveToNextSibling());
		}
	}

	private int ReadTextNodes()
	{
		bool flag = reader.XmlSpace == XmlSpace.Preserve;
		bool flag2 = true;
		int num = 0;
		while (true)
		{
			XmlNodeType xmlNodeType = reader.NodeType;
			if (xmlNodeType <= XmlNodeType.EntityReference)
			{
				if ((uint)(xmlNodeType - 3) > 1u)
				{
					if (xmlNodeType != XmlNodeType.EntityReference)
					{
						break;
					}
					string localName = reader.LocalName;
					if (localName.Length > 0)
					{
						if (localName[0] != '#')
						{
							switch (localName)
							{
							case "lt":
							case "gt":
							case "quot":
							case "apos":
								break;
							default:
								goto IL_0137;
							}
						}
						ExtendRecordBuffer(num);
						FillupCharacterEntityRecord(ref records[num]);
						if (flag2 && !XmlCharType.Instance.IsOnlyWhitespace(records[num].value))
						{
							flag2 = false;
						}
						num++;
						continue;
					}
					goto IL_0137;
				}
				if (flag2 && !XmlCharType.Instance.IsOnlyWhitespace(reader.Value))
				{
					flag2 = false;
				}
			}
			else if ((uint)(xmlNodeType - 13) > 1u)
			{
				if (xmlNodeType != XmlNodeType.EndEntity)
				{
					break;
				}
				reader.Read();
				continue;
			}
			ExtendRecordBuffer(num);
			FillupTextRecord(ref records[num]);
			reader.Read();
			num++;
			continue;
			IL_0137:
			reader.ResolveEntity();
			reader.Read();
		}
		nodeType = ((!flag2) ? XmlNodeType.Text : (flag ? XmlNodeType.SignificantWhitespace : XmlNodeType.Whitespace));
		return num;
	}

	private bool ReadNextSibling()
	{
		if (currentRecord < lastTextNode)
		{
			currentRecord++;
			if (currentRecord == lastTextNode)
			{
				lastTextNode = 0;
			}
			return true;
		}
		currentRecord = 0;
		while (!reader.EOF)
		{
			switch (reader.NodeType)
			{
			case XmlNodeType.Text:
			case XmlNodeType.CDATA:
			case XmlNodeType.EntityReference:
			case XmlNodeType.Whitespace:
			case XmlNodeType.SignificantWhitespace:
			{
				int num = ReadTextNodes();
				if (num != 0)
				{
					lastTextNode = num - 1;
					return true;
				}
				break;
			}
			case XmlNodeType.Element:
				scopeManager.EnterScope();
				numAttributes = ReadElement();
				return true;
			case XmlNodeType.EndElement:
				nodeType = XmlNodeType.EndElement;
				isEmptyElement = false;
				FillupRecord(ref records[0]);
				reader.Read();
				SetRecordEnd(ref records[0]);
				return false;
			default:
				reader.Read();
				break;
			}
		}
		return false;
	}

	private int ReadElement()
	{
		attributesRead = false;
		FillupRecord(ref records[0]);
		nodeType = XmlNodeType.Element;
		isEmptyElement = reader.IsEmptyElement;
		ctxInfo = new ContextInfo(this);
		int num = 1;
		if (reader.MoveToFirstAttribute())
		{
			do
			{
				ExtendRecordBuffer(num);
				if (ReadAttribute(ref records[num]))
				{
					num++;
				}
			}
			while (reader.MoveToNextAttribute());
			reader.MoveToElement();
		}
		reader.Read();
		SetRecordEnd(ref records[0]);
		ctxInfo.lineInfo = BuildLineInfo();
		attributes = null;
		return num - 1;
	}

	public void MoveToElement()
	{
		currentRecord = 0;
	}

	private bool MoveToAttributeBase(int attNum)
	{
		if (0 < attNum && attNum <= numAttributes)
		{
			currentRecord = attNum;
			return true;
		}
		currentRecord = 0;
		return false;
	}

	public bool MoveToLiteralAttribute(int attNum)
	{
		if (0 < attNum && attNum <= numAttributes)
		{
			currentRecord = attNum;
			return true;
		}
		currentRecord = 0;
		return false;
	}

	public bool MoveToXsltAttribute(int attNum, string attName)
	{
		currentRecord = xsltAttributeNumber[attNum];
		return currentRecord != 0;
	}

	public bool IsRequiredAttribute(int attNum)
	{
		return (attributes[attNum].flags & ((compiler.Version == 2) ? XsltLoader.V2Req : XsltLoader.V1Req)) != 0;
	}

	public bool AttributeExists(int attNum, string attName)
	{
		return xsltAttributeNumber[attNum] != 0;
	}

	public bool IsNs(string ns)
	{
		return Ref.Equal(ns, NamespaceUri);
	}

	public bool IsKeyword(string kwd)
	{
		return Ref.Equal(kwd, LocalName);
	}

	public bool IsXsltNamespace()
	{
		return IsNs(atoms.UriXsl);
	}

	public bool IsNullNamespace()
	{
		return IsNs(string.Empty);
	}

	public bool IsXsltAttribute(string kwd)
	{
		if (IsKeyword(kwd))
		{
			return IsNullNamespace();
		}
		return false;
	}

	public bool IsXsltKeyword(string kwd)
	{
		if (IsKeyword(kwd))
		{
			return IsXsltNamespace();
		}
		return false;
	}

	public bool IsExtensionNamespace(string uri)
	{
		return scopeManager.IsExNamespace(uri);
	}

	private void SetVersion(int attVersion)
	{
		MoveToLiteralAttribute(attVersion);
		double num = XPathConvert.StringToDouble(Value);
		if (double.IsNaN(num))
		{
			ReportError("'{1}' is an invalid value for the '{0}' attribute.", atoms.Version, Value);
			num = 1.0;
		}
		SetVersion(num);
	}

	private void SetVersion(double version)
	{
		if (compiler.Version == 0)
		{
			compiler.Version = 1;
		}
		if (compiler.Version == 1)
		{
			scopeManager.BackwardCompatibility = false;
			scopeManager.ForwardCompatibility = version != 1.0;
		}
		else
		{
			scopeManager.BackwardCompatibility = version < 2.0;
			scopeManager.ForwardCompatibility = 2.0 < version;
		}
	}

	public ContextInfo GetAttributes()
	{
		return GetAttributes(noAttributes);
	}

	public ContextInfo GetAttributes(XsltAttribute[] attributes)
	{
		this.attributes = attributes;
		records[0].value = null;
		int attExPrefixes = 0;
		int attExPrefixes2 = 0;
		int xPathDefaultNamespace = 0;
		int defaultCollation = 0;
		int num = 0;
		bool flag = IsXsltNamespace() && IsKeyword(atoms.Output);
		bool flag2 = IsXsltNamespace() && (IsKeyword(atoms.Stylesheet) || IsKeyword(atoms.Transform));
		bool flag3 = compiler.Version == 2;
		for (int i = 0; i < attributes.Length; i++)
		{
			xsltAttributeNumber[i] = 0;
		}
		compiler.EnterForwardsCompatible();
		if (flag2 || (flag3 && !flag))
		{
			for (int j = 1; MoveToAttributeBase(j); j++)
			{
				if (IsNullNamespace() && IsKeyword(atoms.Version))
				{
					SetVersion(j);
					break;
				}
			}
		}
		if (compiler.Version == 0)
		{
			SetVersion(1.0);
		}
		flag3 = compiler.Version == 2;
		int num2 = (flag3 ? (XsltLoader.V2Opt | XsltLoader.V2Req) : (XsltLoader.V1Opt | XsltLoader.V1Req));
		for (int k = 1; MoveToAttributeBase(k); k++)
		{
			if (IsNullNamespace())
			{
				string localName = LocalName;
				int l;
				for (l = 0; l < attributes.Length; l++)
				{
					if (Ref.Equal(localName, attributes[l].name) && (attributes[l].flags & num2) != 0)
					{
						xsltAttributeNumber[l] = k;
						break;
					}
				}
				if (l == attributes.Length)
				{
					if (Ref.Equal(localName, atoms.ExcludeResultPrefixes) && (flag2 || flag3))
					{
						attExPrefixes2 = k;
						continue;
					}
					if (Ref.Equal(localName, atoms.ExtensionElementPrefixes) && (flag2 || flag3))
					{
						attExPrefixes = k;
						continue;
					}
					if (Ref.Equal(localName, atoms.XPathDefaultNamespace) && flag3)
					{
						xPathDefaultNamespace = k;
						continue;
					}
					if (Ref.Equal(localName, atoms.DefaultCollation) && flag3)
					{
						defaultCollation = k;
						continue;
					}
					if (Ref.Equal(localName, atoms.UseWhen) && flag3)
					{
						num = k;
						continue;
					}
					ReportError("'{0}' is an invalid attribute for the '{1}' element.", QualifiedName, records[0].QualifiedName);
				}
			}
			else if (IsXsltNamespace())
			{
				ReportError("'{0}' is an invalid attribute for the '{1}' element.", QualifiedName, records[0].QualifiedName);
			}
		}
		attributesRead = true;
		compiler.ExitForwardsCompatible(ForwardCompatibility);
		InsertExNamespaces(attExPrefixes, ctxInfo, extensions: true);
		InsertExNamespaces(attExPrefixes2, ctxInfo, extensions: false);
		SetXPathDefaultNamespace(xPathDefaultNamespace);
		SetDefaultCollation(defaultCollation);
		if (num != 0)
		{
			ReportNYI(atoms.UseWhen);
		}
		MoveToElement();
		for (int m = 0; m < attributes.Length; m++)
		{
			if (xsltAttributeNumber[m] == 0)
			{
				int flags = attributes[m].flags;
				if ((compiler.Version == 2 && (flags & XsltLoader.V2Req) != 0) || (compiler.Version == 1 && (flags & XsltLoader.V1Req) != 0 && (!ForwardCompatibility || (flags & XsltLoader.V2Req) != 0)))
				{
					ReportError("Missing mandatory attribute '{0}'.", attributes[m].name);
				}
			}
		}
		return ctxInfo;
	}

	public ContextInfo GetLiteralAttributes(bool asStylesheet)
	{
		int num = 0;
		int attExPrefixes = 0;
		int attExPrefixes2 = 0;
		int xPathDefaultNamespace = 0;
		int defaultCollation = 0;
		int num2 = 0;
		for (int i = 1; MoveToLiteralAttribute(i); i++)
		{
			if (IsXsltNamespace())
			{
				string localName = LocalName;
				if (Ref.Equal(localName, atoms.Version))
				{
					num = i;
				}
				else if (Ref.Equal(localName, atoms.ExtensionElementPrefixes))
				{
					attExPrefixes = i;
				}
				else if (Ref.Equal(localName, atoms.ExcludeResultPrefixes))
				{
					attExPrefixes2 = i;
				}
				else if (Ref.Equal(localName, atoms.XPathDefaultNamespace))
				{
					xPathDefaultNamespace = i;
				}
				else if (Ref.Equal(localName, atoms.DefaultCollation))
				{
					defaultCollation = i;
				}
				else if (Ref.Equal(localName, atoms.UseWhen))
				{
					num2 = i;
				}
			}
		}
		attributesRead = true;
		MoveToElement();
		if (num != 0)
		{
			SetVersion(num);
		}
		else if (asStylesheet)
		{
			ReportError((Ref.Equal(NamespaceUri, atoms.UriWdXsl) && Ref.Equal(LocalName, atoms.Stylesheet)) ? "The 'http://www.w3.org/TR/WD-xsl' namespace is no longer supported." : "Stylesheet must start either with an 'xsl:stylesheet' or an 'xsl:transform' element, or with a literal result element that has an 'xsl:version' attribute, where prefix 'xsl' denotes the 'http://www.w3.org/1999/XSL/Transform' namespace.");
			SetVersion(1.0);
		}
		InsertExNamespaces(attExPrefixes, ctxInfo, extensions: true);
		if (!IsExtensionNamespace(records[0].nsUri))
		{
			if (compiler.Version == 2)
			{
				SetXPathDefaultNamespace(xPathDefaultNamespace);
				SetDefaultCollation(defaultCollation);
				if (num2 != 0)
				{
					ReportNYI(atoms.UseWhen);
				}
			}
			InsertExNamespaces(attExPrefixes2, ctxInfo, extensions: false);
		}
		return ctxInfo;
	}

	public void GetVersionAttribute()
	{
		if (compiler.Version == 2)
		{
			for (int i = 1; MoveToAttributeBase(i); i++)
			{
				if (IsNullNamespace() && IsKeyword(atoms.Version))
				{
					SetVersion(i);
					break;
				}
			}
		}
		attributesRead = true;
	}

	private void InsertExNamespaces(int attExPrefixes, ContextInfo ctxInfo, bool extensions)
	{
		if (!MoveToLiteralAttribute(attExPrefixes))
		{
			return;
		}
		string value = Value;
		if (value.Length == 0)
		{
			return;
		}
		if (!extensions && compiler.Version != 1 && value == "#all")
		{
			ctxInfo.nsList = new NsDecl(ctxInfo.nsList, null, null);
			return;
		}
		compiler.EnterForwardsCompatible();
		string[] array = XmlConvert.SplitString(value);
		for (int i = 0; i < array.Length; i++)
		{
			if (array[i] == "#default")
			{
				array[i] = LookupXmlNamespace(string.Empty);
				if (array[i].Length == 0 && compiler.Version != 1 && !BackwardCompatibility)
				{
					ReportError("Value '#default' is used within the 'exclude-result-prefixes' attribute and the parent element of this attribute has no default namespace.");
				}
			}
			else
			{
				array[i] = LookupXmlNamespace(array[i]);
			}
		}
		if (!compiler.ExitForwardsCompatible(ForwardCompatibility))
		{
			return;
		}
		for (int j = 0; j < array.Length; j++)
		{
			if (array[j] != null)
			{
				ctxInfo.nsList = new NsDecl(ctxInfo.nsList, null, array[j]);
				if (extensions)
				{
					scopeManager.AddExNamespace(array[j]);
				}
			}
		}
	}

	private void SetXPathDefaultNamespace(int attNamespace)
	{
		if (MoveToLiteralAttribute(attNamespace) && Value.Length != 0)
		{
			ReportNYI(atoms.XPathDefaultNamespace);
		}
	}

	private void SetDefaultCollation(int attCollation)
	{
		if (MoveToLiteralAttribute(attCollation))
		{
			string[] array = XmlConvert.SplitString(Value);
			int i;
			for (i = 0; i < array.Length && XmlCollation.Create(array[i], throwOnError: false) == null; i++)
			{
			}
			if (i == array.Length)
			{
				ReportErrorFC("The value of an 'default-collation' attribute contains no recognized collation URI.");
			}
			else if (array[i] != "http://www.w3.org/2004/10/xpath-functions/collation/codepoint")
			{
				ReportNYI(atoms.DefaultCollation);
			}
		}
	}

	private static int PositionAdjustment(XmlNodeType nt)
	{
		return nt switch
		{
			XmlNodeType.Element => 1, 
			XmlNodeType.CDATA => 9, 
			XmlNodeType.ProcessingInstruction => 2, 
			XmlNodeType.Comment => 4, 
			XmlNodeType.EndElement => 2, 
			XmlNodeType.EntityReference => 1, 
			_ => 0, 
		};
	}

	public ISourceLineInfo BuildLineInfo()
	{
		return new SourceLineInfo(Uri, Start, End);
	}

	public ISourceLineInfo BuildNameLineInfo()
	{
		if (readerLineInfo == null)
		{
			return BuildLineInfo();
		}
		if (LocalName == null)
		{
			FillupRecord(ref records[currentRecord]);
		}
		Location start = Start;
		int line = start.Line;
		int num = start.Pos + PositionAdjustment(NodeType);
		return new SourceLineInfo(Uri, new Location(line, num), new Location(line, num + QualifiedName.Length));
	}

	public ISourceLineInfo BuildReaderLineInfo()
	{
		Location location = ((readerLineInfo == null) ? new Location(0, 0) : new Location(readerLineInfo.LineNumber, readerLineInfo.LinePosition));
		return new SourceLineInfo(reader.BaseURI, location, location);
	}

	public string LookupXmlNamespace(string prefix)
	{
		string text = scopeManager.LookupNamespace(prefix);
		if (text != null)
		{
			return text;
		}
		if (prefix.Length == 0)
		{
			return string.Empty;
		}
		ReportError("Prefix '{0}' is not defined.", prefix);
		return null;
	}

	public void ReportError(string res, params string[] args)
	{
		compiler.ReportError(BuildNameLineInfo(), res, args);
	}

	public void ReportErrorFC(string res, params string[] args)
	{
		if (!ForwardCompatibility)
		{
			compiler.ReportError(BuildNameLineInfo(), res, args);
		}
	}

	public void ReportWarning(string res, params string[] args)
	{
		compiler.ReportWarning(BuildNameLineInfo(), res, args);
	}

	private void ReportNYI(string arg)
	{
		ReportErrorFC("'{0}' is not yet implemented.", arg);
	}
}
