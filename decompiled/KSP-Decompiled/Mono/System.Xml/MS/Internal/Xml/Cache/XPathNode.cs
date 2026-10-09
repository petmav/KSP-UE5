using System.Xml.XPath;

namespace MS.Internal.Xml.Cache;

internal struct XPathNode
{
	private XPathNodeInfoAtom info;

	private ushort idxSibling;

	private ushort idxParent;

	private ushort idxSimilar;

	private ushort posOffset;

	private uint props;

	private string value;

	private const uint NodeTypeMask = 15u;

	private const uint HasAttributeBit = 16u;

	private const uint HasContentChildBit = 32u;

	private const uint HasElementChildBit = 64u;

	private const uint HasCollapsedTextBit = 128u;

	private const uint AllowShortcutTagBit = 256u;

	private const uint HasNmspDeclsBit = 512u;

	private const uint LineNumberMask = 16776192u;

	private const int LineNumberShift = 10;

	private const int CollapsedPositionShift = 24;

	public const int MaxLineNumberOffset = 16383;

	public const int MaxLinePositionOffset = 65535;

	public const int MaxCollapsedPositionOffset = 255;

	public XPathNodeType NodeType => (XPathNodeType)((int)props & 0xF);

	public string Prefix => info.Prefix;

	public string LocalName => info.LocalName;

	public string Name
	{
		get
		{
			if (Prefix.Length == 0)
			{
				return LocalName;
			}
			return Prefix + ":" + LocalName;
		}
	}

	public string NamespaceUri => info.NamespaceUri;

	public XPathDocument Document => info.Document;

	public string BaseUri => info.BaseUri;

	public int LineNumber => info.LineNumberBase + (int)((props & 0xFFFC00) >> 10);

	public int LinePosition => info.LinePositionBase + posOffset;

	public int CollapsedLinePosition => LinePosition + (int)(props >> 24);

	public XPathNodePageInfo PageInfo => info.PageInfo;

	public bool IsXmlNamespaceNode
	{
		get
		{
			string localName = info.LocalName;
			if (NodeType == XPathNodeType.Namespace && localName.Length == 3)
			{
				return localName == "xml";
			}
			return false;
		}
	}

	public bool HasSibling => idxSibling != 0;

	public bool HasCollapsedText => (props & 0x80) != 0;

	public bool HasAttribute => (props & 0x10) != 0;

	public bool HasContentChild => (props & 0x20) != 0;

	public bool HasElementChild => (props & 0x40) != 0;

	public bool IsAttrNmsp
	{
		get
		{
			XPathNodeType nodeType = NodeType;
			if (nodeType != XPathNodeType.Attribute)
			{
				return nodeType == XPathNodeType.Namespace;
			}
			return true;
		}
	}

	public bool IsText => XPathNavigator.IsText(NodeType);

	public bool HasNamespaceDecls
	{
		get
		{
			return (props & 0x200) != 0;
		}
		set
		{
			if (value)
			{
				props |= 512u;
			}
			else
			{
				props &= 255u;
			}
		}
	}

	public bool AllowShortcutTag => (props & 0x100) != 0;

	public int LocalNameHashCode => info.LocalNameHashCode;

	public string Value => value;

	public int GetRoot(out XPathNode[] pageNode)
	{
		return info.Document.GetRootNode(out pageNode);
	}

	public int GetParent(out XPathNode[] pageNode)
	{
		pageNode = info.ParentPage;
		return idxParent;
	}

	public int GetSibling(out XPathNode[] pageNode)
	{
		pageNode = info.SiblingPage;
		return idxSibling;
	}

	public int GetSimilarElement(out XPathNode[] pageNode)
	{
		pageNode = info.SimilarElementPage;
		return idxSimilar;
	}

	public bool NameMatch(string localName, string namespaceName)
	{
		if ((object)info.LocalName == localName)
		{
			return info.NamespaceUri == namespaceName;
		}
		return false;
	}

	public bool ElementMatch(string localName, string namespaceName)
	{
		if (NodeType == XPathNodeType.Element && (object)info.LocalName == localName)
		{
			return info.NamespaceUri == namespaceName;
		}
		return false;
	}

	public void Create(XPathNodePageInfo pageInfo)
	{
		info = new XPathNodeInfoAtom(pageInfo);
	}

	public void Create(XPathNodeInfoAtom info, XPathNodeType xptyp, int idxParent)
	{
		this.info = info;
		props = (uint)xptyp;
		this.idxParent = (ushort)idxParent;
	}

	public void SetLineInfoOffsets(int lineNumOffset, int linePosOffset)
	{
		props |= (uint)(lineNumOffset << 10);
		posOffset = (ushort)linePosOffset;
	}

	public void SetCollapsedLineInfoOffset(int posOffset)
	{
		props |= (uint)(posOffset << 24);
	}

	public void SetValue(string value)
	{
		this.value = value;
	}

	public void SetEmptyValue(bool allowShortcutTag)
	{
		value = string.Empty;
		if (allowShortcutTag)
		{
			props |= 256u;
		}
	}

	public void SetCollapsedValue(string value)
	{
		this.value = value;
		props |= 160u;
	}

	public void SetParentProperties(XPathNodeType xptyp)
	{
		if (xptyp == XPathNodeType.Attribute)
		{
			props |= 16u;
			return;
		}
		props |= 32u;
		if (xptyp == XPathNodeType.Element)
		{
			props |= 64u;
		}
	}

	public void SetSibling(XPathNodeInfoTable infoTable, XPathNode[] pageSibling, int idxSibling)
	{
		this.idxSibling = (ushort)idxSibling;
		if (pageSibling != info.SiblingPage)
		{
			info = infoTable.Create(info.LocalName, info.NamespaceUri, info.Prefix, info.BaseUri, info.ParentPage, pageSibling, info.SimilarElementPage, info.Document, info.LineNumberBase, info.LinePositionBase);
		}
	}

	public void SetSimilarElement(XPathNodeInfoTable infoTable, XPathNode[] pageSimilar, int idxSimilar)
	{
		this.idxSimilar = (ushort)idxSimilar;
		if (pageSimilar != info.SimilarElementPage)
		{
			info = infoTable.Create(info.LocalName, info.NamespaceUri, info.Prefix, info.BaseUri, info.ParentPage, info.SiblingPage, pageSimilar, info.Document, info.LineNumberBase, info.LinePositionBase);
		}
	}
}
