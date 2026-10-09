namespace System.Xml.Linq;

internal class XNodeReader : XmlReader, IXmlLineInfo
{
	private object source;

	private object parent;

	private ReadState state;

	private XNode root;

	private XmlNameTable nameTable;

	private bool omitDuplicateNamespaces;

	private IDtdInfo dtdInfo;

	private bool dtdInfoInitialized;

	public override int AttributeCount
	{
		get
		{
			if (!IsInteractive)
			{
				return 0;
			}
			int num = 0;
			XElement elementInAttributeScope = GetElementInAttributeScope();
			if (elementInAttributeScope != null)
			{
				XAttribute xAttribute = elementInAttributeScope.lastAttr;
				if (xAttribute != null)
				{
					do
					{
						xAttribute = xAttribute.next;
						if (!omitDuplicateNamespaces || !IsDuplicateNamespaceAttribute(xAttribute))
						{
							num++;
						}
					}
					while (xAttribute != elementInAttributeScope.lastAttr);
				}
			}
			return num;
		}
	}

	public override string BaseURI
	{
		get
		{
			if (source is XObject xObject)
			{
				return xObject.BaseUri;
			}
			if (parent is XObject xObject2)
			{
				return xObject2.BaseUri;
			}
			return string.Empty;
		}
	}

	public override int Depth
	{
		get
		{
			if (!IsInteractive)
			{
				return 0;
			}
			if (source is XObject o)
			{
				return GetDepth(o);
			}
			if (parent is XObject o2)
			{
				return GetDepth(o2) + 1;
			}
			return 0;
		}
	}

	public override bool EOF => state == ReadState.EndOfFile;

	public override bool HasAttributes
	{
		get
		{
			if (!IsInteractive)
			{
				return false;
			}
			XElement elementInAttributeScope = GetElementInAttributeScope();
			if (elementInAttributeScope != null && elementInAttributeScope.lastAttr != null)
			{
				if (omitDuplicateNamespaces)
				{
					return GetFirstNonDuplicateNamespaceAttribute(elementInAttributeScope.lastAttr.next) != null;
				}
				return true;
			}
			return false;
		}
	}

	public override bool HasValue
	{
		get
		{
			if (!IsInteractive)
			{
				return false;
			}
			if (source is XObject xObject)
			{
				switch (xObject.NodeType)
				{
				case XmlNodeType.Attribute:
				case XmlNodeType.Text:
				case XmlNodeType.CDATA:
				case XmlNodeType.ProcessingInstruction:
				case XmlNodeType.Comment:
				case XmlNodeType.DocumentType:
					return true;
				default:
					return false;
				}
			}
			return true;
		}
	}

	public override bool IsEmptyElement
	{
		get
		{
			if (!IsInteractive)
			{
				return false;
			}
			if (source is XElement xElement)
			{
				return xElement.IsEmpty;
			}
			return false;
		}
	}

	public override string LocalName => nameTable.Add(GetLocalName());

	public override string Name
	{
		get
		{
			string prefix = GetPrefix();
			if (prefix.Length == 0)
			{
				return nameTable.Add(GetLocalName());
			}
			return nameTable.Add(prefix + ":" + GetLocalName());
		}
	}

	public override string NamespaceURI => nameTable.Add(GetNamespaceURI());

	public override XmlNameTable NameTable => nameTable;

	public override XmlNodeType NodeType
	{
		get
		{
			if (!IsInteractive)
			{
				return XmlNodeType.None;
			}
			if (source is XObject xObject)
			{
				if (IsEndElement)
				{
					return XmlNodeType.EndElement;
				}
				XmlNodeType nodeType = xObject.NodeType;
				if (nodeType != XmlNodeType.Text)
				{
					return nodeType;
				}
				if (xObject.parent != null && xObject.parent.parent == null && xObject.parent is XDocument)
				{
					return XmlNodeType.Whitespace;
				}
				return XmlNodeType.Text;
			}
			if (parent is XDocument)
			{
				return XmlNodeType.Whitespace;
			}
			return XmlNodeType.Text;
		}
	}

	public override string Prefix => nameTable.Add(GetPrefix());

	public override ReadState ReadState => state;

	public override XmlReaderSettings Settings => new XmlReaderSettings
	{
		CheckCharacters = false
	};

	public override string Value
	{
		get
		{
			if (!IsInteractive)
			{
				return string.Empty;
			}
			if (source is XObject xObject)
			{
				switch (xObject.NodeType)
				{
				case XmlNodeType.Attribute:
					return ((XAttribute)xObject).Value;
				case XmlNodeType.Text:
				case XmlNodeType.CDATA:
					return ((XText)xObject).Value;
				case XmlNodeType.Comment:
					return ((XComment)xObject).Value;
				case XmlNodeType.ProcessingInstruction:
					return ((XProcessingInstruction)xObject).Data;
				case XmlNodeType.DocumentType:
					return ((XDocumentType)xObject).InternalSubset;
				default:
					return string.Empty;
				}
			}
			return (string)source;
		}
	}

	public override string XmlLang
	{
		get
		{
			if (!IsInteractive)
			{
				return string.Empty;
			}
			XElement xElement = GetElementInScope();
			if (xElement != null)
			{
				XName name = XNamespace.Xml.GetName("lang");
				do
				{
					XAttribute xAttribute = xElement.Attribute(name);
					if (xAttribute != null)
					{
						return xAttribute.Value;
					}
					xElement = xElement.parent as XElement;
				}
				while (xElement != null);
			}
			return string.Empty;
		}
	}

	public override XmlSpace XmlSpace
	{
		get
		{
			if (!IsInteractive)
			{
				return XmlSpace.None;
			}
			XElement xElement = GetElementInScope();
			if (xElement != null)
			{
				XName name = XNamespace.Xml.GetName("space");
				do
				{
					XAttribute xAttribute = xElement.Attribute(name);
					if (xAttribute != null)
					{
						string text = xAttribute.Value.Trim(' ', '\t', '\n', '\r');
						if (text == "preserve")
						{
							return XmlSpace.Preserve;
						}
						if (text == "default")
						{
							return XmlSpace.Default;
						}
					}
					xElement = xElement.parent as XElement;
				}
				while (xElement != null);
			}
			return XmlSpace.None;
		}
	}

	internal override IDtdInfo DtdInfo
	{
		get
		{
			if (dtdInfoInitialized)
			{
				return dtdInfo;
			}
			dtdInfoInitialized = true;
			XDocumentType xDocumentType = source as XDocumentType;
			if (xDocumentType == null)
			{
				for (XNode xNode = root; xNode != null; xNode = xNode.parent)
				{
					if (xNode is XDocument xDocument)
					{
						xDocumentType = xDocument.DocumentType;
						break;
					}
				}
			}
			if (xDocumentType != null)
			{
				dtdInfo = xDocumentType.DtdInfo;
			}
			return dtdInfo;
		}
	}

	int IXmlLineInfo.LineNumber
	{
		get
		{
			if (IsEndElement)
			{
				if (source is XElement xElement)
				{
					LineInfoEndElementAnnotation lineInfoEndElementAnnotation = xElement.Annotation<LineInfoEndElementAnnotation>();
					if (lineInfoEndElementAnnotation != null)
					{
						return lineInfoEndElementAnnotation.lineNumber;
					}
				}
			}
			else if (source is IXmlLineInfo xmlLineInfo)
			{
				return xmlLineInfo.LineNumber;
			}
			return 0;
		}
	}

	int IXmlLineInfo.LinePosition
	{
		get
		{
			if (IsEndElement)
			{
				if (source is XElement xElement)
				{
					LineInfoEndElementAnnotation lineInfoEndElementAnnotation = xElement.Annotation<LineInfoEndElementAnnotation>();
					if (lineInfoEndElementAnnotation != null)
					{
						return lineInfoEndElementAnnotation.linePosition;
					}
				}
			}
			else if (source is IXmlLineInfo xmlLineInfo)
			{
				return xmlLineInfo.LinePosition;
			}
			return 0;
		}
	}

	private bool IsEndElement
	{
		get
		{
			return parent == source;
		}
		set
		{
			parent = (value ? source : null);
		}
	}

	private bool IsInteractive => state == ReadState.Interactive;

	internal XNodeReader(XNode node, XmlNameTable nameTable, ReaderOptions options)
	{
		source = node;
		root = node;
		this.nameTable = ((nameTable != null) ? nameTable : CreateNameTable());
		omitDuplicateNamespaces = (((options & ReaderOptions.OmitDuplicateNamespaces) != 0) ? true : false);
	}

	internal XNodeReader(XNode node, XmlNameTable nameTable)
		: this(node, nameTable, ((node.GetSaveOptionsFromAnnotations() & SaveOptions.OmitDuplicateNamespaces) != 0) ? ReaderOptions.OmitDuplicateNamespaces : ReaderOptions.None)
	{
	}

	private static int GetDepth(XObject o)
	{
		int num = 0;
		while (o.parent != null)
		{
			num++;
			o = o.parent;
		}
		if (o is XDocument)
		{
			num--;
		}
		return num;
	}

	private string GetLocalName()
	{
		if (!IsInteractive)
		{
			return string.Empty;
		}
		if (source is XElement xElement)
		{
			return xElement.Name.LocalName;
		}
		if (source is XAttribute xAttribute)
		{
			return xAttribute.Name.LocalName;
		}
		if (source is XProcessingInstruction xProcessingInstruction)
		{
			return xProcessingInstruction.Target;
		}
		if (source is XDocumentType xDocumentType)
		{
			return xDocumentType.Name;
		}
		return string.Empty;
	}

	private string GetNamespaceURI()
	{
		if (!IsInteractive)
		{
			return string.Empty;
		}
		if (source is XElement xElement)
		{
			return xElement.Name.NamespaceName;
		}
		if (source is XAttribute xAttribute)
		{
			string namespaceName = xAttribute.Name.NamespaceName;
			if (namespaceName.Length == 0 && xAttribute.Name.LocalName == "xmlns")
			{
				return "http://www.w3.org/2000/xmlns/";
			}
			return namespaceName;
		}
		return string.Empty;
	}

	private string GetPrefix()
	{
		if (!IsInteractive)
		{
			return string.Empty;
		}
		if (source is XElement xElement)
		{
			string prefixOfNamespace = xElement.GetPrefixOfNamespace(xElement.Name.Namespace);
			if (prefixOfNamespace != null)
			{
				return prefixOfNamespace;
			}
			return string.Empty;
		}
		if (source is XAttribute xAttribute)
		{
			string prefixOfNamespace2 = xAttribute.GetPrefixOfNamespace(xAttribute.Name.Namespace);
			if (prefixOfNamespace2 != null)
			{
				return prefixOfNamespace2;
			}
		}
		return string.Empty;
	}

	public override void Close()
	{
		source = null;
		parent = null;
		root = null;
		state = ReadState.Closed;
	}

	public override string GetAttribute(string name)
	{
		if (!IsInteractive)
		{
			return null;
		}
		XElement elementInAttributeScope = GetElementInAttributeScope();
		if (elementInAttributeScope != null)
		{
			GetNameInAttributeScope(name, elementInAttributeScope, out var localName, out var namespaceName);
			XAttribute xAttribute = elementInAttributeScope.lastAttr;
			if (xAttribute != null)
			{
				do
				{
					xAttribute = xAttribute.next;
					if (xAttribute.Name.LocalName == localName && xAttribute.Name.NamespaceName == namespaceName)
					{
						if (omitDuplicateNamespaces && IsDuplicateNamespaceAttribute(xAttribute))
						{
							return null;
						}
						return xAttribute.Value;
					}
				}
				while (xAttribute != elementInAttributeScope.lastAttr);
			}
			return null;
		}
		if (source is XDocumentType xDocumentType)
		{
			if (name == "PUBLIC")
			{
				return xDocumentType.PublicId;
			}
			if (name == "SYSTEM")
			{
				return xDocumentType.SystemId;
			}
		}
		return null;
	}

	public override string GetAttribute(string localName, string namespaceName)
	{
		if (!IsInteractive)
		{
			return null;
		}
		XElement elementInAttributeScope = GetElementInAttributeScope();
		if (elementInAttributeScope != null)
		{
			if (localName == "xmlns")
			{
				if (namespaceName != null && namespaceName.Length == 0)
				{
					return null;
				}
				if (namespaceName == "http://www.w3.org/2000/xmlns/")
				{
					namespaceName = string.Empty;
				}
			}
			XAttribute xAttribute = elementInAttributeScope.lastAttr;
			if (xAttribute != null)
			{
				do
				{
					xAttribute = xAttribute.next;
					if (xAttribute.Name.LocalName == localName && xAttribute.Name.NamespaceName == namespaceName)
					{
						if (omitDuplicateNamespaces && IsDuplicateNamespaceAttribute(xAttribute))
						{
							return null;
						}
						return xAttribute.Value;
					}
				}
				while (xAttribute != elementInAttributeScope.lastAttr);
			}
		}
		return null;
	}

	public override string GetAttribute(int index)
	{
		if (!IsInteractive)
		{
			return null;
		}
		if (index < 0)
		{
			return null;
		}
		XElement elementInAttributeScope = GetElementInAttributeScope();
		if (elementInAttributeScope != null)
		{
			XAttribute xAttribute = elementInAttributeScope.lastAttr;
			if (xAttribute != null)
			{
				do
				{
					xAttribute = xAttribute.next;
					if ((!omitDuplicateNamespaces || !IsDuplicateNamespaceAttribute(xAttribute)) && index-- == 0)
					{
						return xAttribute.Value;
					}
				}
				while (xAttribute != elementInAttributeScope.lastAttr);
			}
		}
		return null;
	}

	public override string LookupNamespace(string prefix)
	{
		if (!IsInteractive)
		{
			return null;
		}
		if (prefix == null)
		{
			return null;
		}
		XElement elementInScope = GetElementInScope();
		if (elementInScope != null)
		{
			XNamespace xNamespace = ((prefix.Length == 0) ? elementInScope.GetDefaultNamespace() : elementInScope.GetNamespaceOfPrefix(prefix));
			if (xNamespace != null)
			{
				return nameTable.Add(xNamespace.NamespaceName);
			}
		}
		return null;
	}

	public override bool MoveToAttribute(string name)
	{
		if (!IsInteractive)
		{
			return false;
		}
		XElement elementInAttributeScope = GetElementInAttributeScope();
		if (elementInAttributeScope != null)
		{
			GetNameInAttributeScope(name, elementInAttributeScope, out var localName, out var namespaceName);
			XAttribute xAttribute = elementInAttributeScope.lastAttr;
			if (xAttribute != null)
			{
				do
				{
					xAttribute = xAttribute.next;
					if (xAttribute.Name.LocalName == localName && xAttribute.Name.NamespaceName == namespaceName)
					{
						if (omitDuplicateNamespaces && IsDuplicateNamespaceAttribute(xAttribute))
						{
							return false;
						}
						source = xAttribute;
						parent = null;
						return true;
					}
				}
				while (xAttribute != elementInAttributeScope.lastAttr);
			}
		}
		return false;
	}

	public override bool MoveToAttribute(string localName, string namespaceName)
	{
		if (!IsInteractive)
		{
			return false;
		}
		XElement elementInAttributeScope = GetElementInAttributeScope();
		if (elementInAttributeScope != null)
		{
			if (localName == "xmlns")
			{
				if (namespaceName != null && namespaceName.Length == 0)
				{
					return false;
				}
				if (namespaceName == "http://www.w3.org/2000/xmlns/")
				{
					namespaceName = string.Empty;
				}
			}
			XAttribute xAttribute = elementInAttributeScope.lastAttr;
			if (xAttribute != null)
			{
				do
				{
					xAttribute = xAttribute.next;
					if (xAttribute.Name.LocalName == localName && xAttribute.Name.NamespaceName == namespaceName)
					{
						if (omitDuplicateNamespaces && IsDuplicateNamespaceAttribute(xAttribute))
						{
							return false;
						}
						source = xAttribute;
						parent = null;
						return true;
					}
				}
				while (xAttribute != elementInAttributeScope.lastAttr);
			}
		}
		return false;
	}

	public override void MoveToAttribute(int index)
	{
		if (!IsInteractive)
		{
			return;
		}
		if (index < 0)
		{
			throw new ArgumentOutOfRangeException("index");
		}
		XElement elementInAttributeScope = GetElementInAttributeScope();
		if (elementInAttributeScope != null)
		{
			XAttribute xAttribute = elementInAttributeScope.lastAttr;
			if (xAttribute != null)
			{
				do
				{
					xAttribute = xAttribute.next;
					if ((!omitDuplicateNamespaces || !IsDuplicateNamespaceAttribute(xAttribute)) && index-- == 0)
					{
						source = xAttribute;
						parent = null;
						return;
					}
				}
				while (xAttribute != elementInAttributeScope.lastAttr);
			}
		}
		throw new ArgumentOutOfRangeException("index");
	}

	public override bool MoveToElement()
	{
		if (!IsInteractive)
		{
			return false;
		}
		XAttribute xAttribute = source as XAttribute;
		if (xAttribute == null)
		{
			xAttribute = parent as XAttribute;
		}
		if (xAttribute != null && xAttribute.parent != null)
		{
			source = xAttribute.parent;
			parent = null;
			return true;
		}
		return false;
	}

	public override bool MoveToFirstAttribute()
	{
		if (!IsInteractive)
		{
			return false;
		}
		XElement elementInAttributeScope = GetElementInAttributeScope();
		if (elementInAttributeScope != null && elementInAttributeScope.lastAttr != null)
		{
			if (omitDuplicateNamespaces)
			{
				object firstNonDuplicateNamespaceAttribute = GetFirstNonDuplicateNamespaceAttribute(elementInAttributeScope.lastAttr.next);
				if (firstNonDuplicateNamespaceAttribute == null)
				{
					return false;
				}
				source = firstNonDuplicateNamespaceAttribute;
			}
			else
			{
				source = elementInAttributeScope.lastAttr.next;
			}
			return true;
		}
		return false;
	}

	public override bool MoveToNextAttribute()
	{
		if (!IsInteractive)
		{
			return false;
		}
		if (source is XElement xElement)
		{
			if (IsEndElement)
			{
				return false;
			}
			if (xElement.lastAttr != null)
			{
				if (omitDuplicateNamespaces)
				{
					object firstNonDuplicateNamespaceAttribute = GetFirstNonDuplicateNamespaceAttribute(xElement.lastAttr.next);
					if (firstNonDuplicateNamespaceAttribute == null)
					{
						return false;
					}
					source = firstNonDuplicateNamespaceAttribute;
				}
				else
				{
					source = xElement.lastAttr.next;
				}
				return true;
			}
			return false;
		}
		XAttribute xAttribute = source as XAttribute;
		if (xAttribute == null)
		{
			xAttribute = parent as XAttribute;
		}
		if (xAttribute != null && xAttribute.parent != null && ((XElement)xAttribute.parent).lastAttr != xAttribute)
		{
			if (omitDuplicateNamespaces)
			{
				object firstNonDuplicateNamespaceAttribute2 = GetFirstNonDuplicateNamespaceAttribute(xAttribute.next);
				if (firstNonDuplicateNamespaceAttribute2 == null)
				{
					return false;
				}
				source = firstNonDuplicateNamespaceAttribute2;
			}
			else
			{
				source = xAttribute.next;
			}
			parent = null;
			return true;
		}
		return false;
	}

	public override bool Read()
	{
		switch (state)
		{
		case ReadState.Initial:
			state = ReadState.Interactive;
			if (source is XDocument d)
			{
				return ReadIntoDocument(d);
			}
			return true;
		case ReadState.Interactive:
			return Read(skipContent: false);
		default:
			return false;
		}
	}

	public override bool ReadAttributeValue()
	{
		if (!IsInteractive)
		{
			return false;
		}
		if (source is XAttribute a)
		{
			return ReadIntoAttribute(a);
		}
		return false;
	}

	public override bool ReadToDescendant(string localName, string namespaceName)
	{
		if (!IsInteractive)
		{
			return false;
		}
		MoveToElement();
		if (source is XElement { IsEmpty: false } xElement)
		{
			if (IsEndElement)
			{
				return false;
			}
			foreach (XElement item in xElement.Descendants())
			{
				if (item.Name.LocalName == localName && item.Name.NamespaceName == namespaceName)
				{
					source = item;
					return true;
				}
			}
			IsEndElement = true;
		}
		return false;
	}

	public override bool ReadToFollowing(string localName, string namespaceName)
	{
		while (Read())
		{
			if (source is XElement xElement && !IsEndElement && xElement.Name.LocalName == localName && xElement.Name.NamespaceName == namespaceName)
			{
				return true;
			}
		}
		return false;
	}

	public override bool ReadToNextSibling(string localName, string namespaceName)
	{
		if (!IsInteractive)
		{
			return false;
		}
		MoveToElement();
		if (source != root)
		{
			if (source is XNode xNode)
			{
				foreach (XElement item in xNode.ElementsAfterSelf())
				{
					if (item.Name.LocalName == localName && item.Name.NamespaceName == namespaceName)
					{
						source = item;
						IsEndElement = false;
						return true;
					}
				}
				if (xNode.parent is XElement)
				{
					source = xNode.parent;
					IsEndElement = true;
					return false;
				}
			}
			else if (parent is XElement)
			{
				source = parent;
				parent = null;
				IsEndElement = true;
				return false;
			}
		}
		return ReadToEnd();
	}

	public override void ResolveEntity()
	{
	}

	public override void Skip()
	{
		if (IsInteractive)
		{
			Read(skipContent: true);
		}
	}

	bool IXmlLineInfo.HasLineInfo()
	{
		if (IsEndElement)
		{
			if (source is XElement xElement)
			{
				return xElement.Annotation<LineInfoEndElementAnnotation>() != null;
			}
		}
		else if (source is IXmlLineInfo xmlLineInfo)
		{
			return xmlLineInfo.HasLineInfo();
		}
		return false;
	}

	private static XmlNameTable CreateNameTable()
	{
		NameTable obj = new NameTable();
		obj.Add(string.Empty);
		obj.Add("http://www.w3.org/2000/xmlns/");
		obj.Add("http://www.w3.org/XML/1998/namespace");
		return obj;
	}

	private XElement GetElementInAttributeScope()
	{
		if (source is XElement result)
		{
			if (IsEndElement)
			{
				return null;
			}
			return result;
		}
		if (source is XAttribute xAttribute)
		{
			return (XElement)xAttribute.parent;
		}
		if (parent is XAttribute xAttribute2)
		{
			return (XElement)xAttribute2.parent;
		}
		return null;
	}

	private XElement GetElementInScope()
	{
		if (source is XElement result)
		{
			return result;
		}
		if (source is XNode xNode)
		{
			return xNode.parent as XElement;
		}
		if (source is XAttribute xAttribute)
		{
			return (XElement)xAttribute.parent;
		}
		if (parent is XElement result2)
		{
			return result2;
		}
		if (parent is XAttribute xAttribute2)
		{
			return (XElement)xAttribute2.parent;
		}
		return null;
	}

	private static void GetNameInAttributeScope(string qualifiedName, XElement e, out string localName, out string namespaceName)
	{
		if (qualifiedName != null && qualifiedName.Length != 0)
		{
			int num = qualifiedName.IndexOf(':');
			if (num != 0 && num != qualifiedName.Length - 1)
			{
				if (num == -1)
				{
					localName = qualifiedName;
					namespaceName = string.Empty;
					return;
				}
				XNamespace namespaceOfPrefix = e.GetNamespaceOfPrefix(qualifiedName.Substring(0, num));
				if (namespaceOfPrefix != null)
				{
					localName = qualifiedName.Substring(num + 1, qualifiedName.Length - num - 1);
					namespaceName = namespaceOfPrefix.NamespaceName;
					return;
				}
			}
		}
		localName = null;
		namespaceName = null;
	}

	private bool Read(bool skipContent)
	{
		if (source is XElement xElement)
		{
			if (xElement.IsEmpty || IsEndElement || skipContent)
			{
				return ReadOverNode(xElement);
			}
			return ReadIntoElement(xElement);
		}
		if (source is XNode n)
		{
			return ReadOverNode(n);
		}
		if (source is XAttribute a)
		{
			return ReadOverAttribute(a, skipContent);
		}
		return ReadOverText(skipContent);
	}

	private bool ReadIntoDocument(XDocument d)
	{
		if (d.content is XNode xNode)
		{
			source = xNode.next;
			return true;
		}
		if (d.content is string { Length: >0 } text)
		{
			source = text;
			parent = d;
			return true;
		}
		return ReadToEnd();
	}

	private bool ReadIntoElement(XElement e)
	{
		if (e.content is XNode xNode)
		{
			source = xNode.next;
			return true;
		}
		if (e.content is string text)
		{
			if (text.Length > 0)
			{
				source = text;
				parent = e;
			}
			else
			{
				source = e;
				IsEndElement = true;
			}
			return true;
		}
		return ReadToEnd();
	}

	private bool ReadIntoAttribute(XAttribute a)
	{
		source = a.value;
		parent = a;
		return true;
	}

	private bool ReadOverAttribute(XAttribute a, bool skipContent)
	{
		XElement xElement = (XElement)a.parent;
		if (xElement != null)
		{
			if (xElement.IsEmpty || skipContent)
			{
				return ReadOverNode(xElement);
			}
			return ReadIntoElement(xElement);
		}
		return ReadToEnd();
	}

	private bool ReadOverNode(XNode n)
	{
		if (n == root)
		{
			return ReadToEnd();
		}
		XNode next = n.next;
		if (next == null || next == n || n == n.parent.content)
		{
			if (n.parent == null || (n.parent.parent == null && n.parent is XDocument))
			{
				return ReadToEnd();
			}
			source = n.parent;
			IsEndElement = true;
		}
		else
		{
			source = next;
			IsEndElement = false;
		}
		return true;
	}

	private bool ReadOverText(bool skipContent)
	{
		if (parent is XElement)
		{
			source = parent;
			parent = null;
			IsEndElement = true;
			return true;
		}
		if (parent is XAttribute)
		{
			XAttribute a = (XAttribute)parent;
			parent = null;
			return ReadOverAttribute(a, skipContent);
		}
		return ReadToEnd();
	}

	private bool ReadToEnd()
	{
		state = ReadState.EndOfFile;
		return false;
	}

	private bool IsDuplicateNamespaceAttribute(XAttribute candidateAttribute)
	{
		if (!candidateAttribute.IsNamespaceDeclaration)
		{
			return false;
		}
		return IsDuplicateNamespaceAttributeInner(candidateAttribute);
	}

	private bool IsDuplicateNamespaceAttributeInner(XAttribute candidateAttribute)
	{
		if (candidateAttribute.Name.LocalName == "xml")
		{
			return true;
		}
		XElement xElement = candidateAttribute.parent as XElement;
		if (xElement == root || xElement == null)
		{
			return false;
		}
		for (xElement = xElement.parent as XElement; xElement != null; xElement = xElement.parent as XElement)
		{
			XAttribute xAttribute = xElement.lastAttr;
			if (xAttribute != null)
			{
				do
				{
					if (xAttribute.name == candidateAttribute.name)
					{
						if (xAttribute.Value == candidateAttribute.Value)
						{
							return true;
						}
						return false;
					}
					xAttribute = xAttribute.next;
				}
				while (xAttribute != xElement.lastAttr);
			}
			if (xElement == root)
			{
				return false;
			}
		}
		return false;
	}

	private XAttribute GetFirstNonDuplicateNamespaceAttribute(XAttribute candidate)
	{
		if (!IsDuplicateNamespaceAttribute(candidate))
		{
			return candidate;
		}
		if (candidate.parent is XElement xElement && candidate != xElement.lastAttr)
		{
			do
			{
				candidate = candidate.next;
				if (!IsDuplicateNamespaceAttribute(candidate))
				{
					return candidate;
				}
			}
			while (candidate != xElement.lastAttr);
		}
		return null;
	}
}
