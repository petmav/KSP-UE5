using System.Text;
using System.Xml;
using System.Xml.XPath;

namespace MS.Internal.Xml.Cache;

internal sealed class XPathDocumentNavigator : XPathNavigator, IXmlLineInfo
{
	private XPathNode[] pageCurrent;

	private XPathNode[] pageParent;

	private int idxCurrent;

	private int idxParent;

	private string atomizedLocalName;

	public override string Value
	{
		get
		{
			string value = pageCurrent[idxCurrent].Value;
			if (value != null)
			{
				return value;
			}
			if (idxParent != 0)
			{
				return pageParent[idxParent].Value;
			}
			string text = string.Empty;
			StringBuilder stringBuilder = null;
			XPathNode[] pageNode;
			XPathNode[] array = (pageNode = pageCurrent);
			int idxNode;
			int num = (idxNode = idxCurrent);
			if (!XPathNodeHelper.GetNonDescendant(ref pageNode, ref idxNode))
			{
				pageNode = null;
				idxNode = 0;
			}
			while (XPathNodeHelper.GetTextFollowing(ref array, ref num, pageNode, idxNode))
			{
				if (text.Length == 0)
				{
					text = array[num].Value;
					continue;
				}
				if (stringBuilder == null)
				{
					stringBuilder = new StringBuilder();
					stringBuilder.Append(text);
				}
				stringBuilder.Append(array[num].Value);
			}
			if (stringBuilder == null)
			{
				return text;
			}
			return stringBuilder.ToString();
		}
	}

	public override XPathNodeType NodeType => pageCurrent[idxCurrent].NodeType;

	public override string LocalName => pageCurrent[idxCurrent].LocalName;

	public override string NamespaceURI => pageCurrent[idxCurrent].NamespaceUri;

	public override string Name => pageCurrent[idxCurrent].Name;

	public override string Prefix => pageCurrent[idxCurrent].Prefix;

	public override string BaseURI
	{
		get
		{
			XPathNode[] pageNode;
			int parent;
			if (idxParent != 0)
			{
				pageNode = pageParent;
				parent = idxParent;
			}
			else
			{
				pageNode = pageCurrent;
				parent = idxCurrent;
			}
			do
			{
				XPathNodeType nodeType = pageNode[parent].NodeType;
				if ((uint)nodeType <= 1u || nodeType == XPathNodeType.ProcessingInstruction)
				{
					return pageNode[parent].BaseUri;
				}
				parent = pageNode[parent].GetParent(out pageNode);
			}
			while (parent != 0);
			return string.Empty;
		}
	}

	public override bool IsEmptyElement => pageCurrent[idxCurrent].AllowShortcutTag;

	public override XmlNameTable NameTable => pageCurrent[idxCurrent].Document.NameTable;

	public override bool HasAttributes => pageCurrent[idxCurrent].HasAttribute;

	public override bool HasChildren => pageCurrent[idxCurrent].HasContentChild;

	internal override string UniqueId
	{
		get
		{
			char[] array = new char[16];
			int length = 0;
			array[length++] = XPathNavigator.NodeTypeLetter[(int)pageCurrent[idxCurrent].NodeType];
			int num;
			if (idxParent != 0)
			{
				num = (pageParent[0].PageInfo.PageNumber - 1 << 16) | (idxParent - 1);
				do
				{
					array[length++] = XPathNavigator.UniqueIdTbl[num & 0x1F];
					num >>= 5;
				}
				while (num != 0);
				array[length++] = '0';
			}
			num = (pageCurrent[0].PageInfo.PageNumber - 1 << 16) | (idxCurrent - 1);
			do
			{
				array[length++] = XPathNavigator.UniqueIdTbl[num & 0x1F];
				num >>= 5;
			}
			while (num != 0);
			return new string(array, 0, length);
		}
	}

	public override object UnderlyingObject => Clone();

	public int LineNumber
	{
		get
		{
			if (idxParent != 0 && NodeType == XPathNodeType.Text)
			{
				return pageParent[idxParent].LineNumber;
			}
			return pageCurrent[idxCurrent].LineNumber;
		}
	}

	public int LinePosition
	{
		get
		{
			if (idxParent != 0 && NodeType == XPathNodeType.Text)
			{
				return pageParent[idxParent].CollapsedLinePosition;
			}
			return pageCurrent[idxCurrent].LinePosition;
		}
	}

	public XPathDocumentNavigator(XPathNode[] pageCurrent, int idxCurrent, XPathNode[] pageParent, int idxParent)
	{
		this.pageCurrent = pageCurrent;
		this.pageParent = pageParent;
		this.idxCurrent = idxCurrent;
		this.idxParent = idxParent;
	}

	public XPathDocumentNavigator(XPathDocumentNavigator nav)
		: this(nav.pageCurrent, nav.idxCurrent, nav.pageParent, nav.idxParent)
	{
		atomizedLocalName = nav.atomizedLocalName;
	}

	public override XPathNavigator Clone()
	{
		return new XPathDocumentNavigator(pageCurrent, idxCurrent, pageParent, idxParent);
	}

	public override bool MoveToFirstAttribute()
	{
		XPathNode[] array = pageCurrent;
		int num = idxCurrent;
		if (XPathNodeHelper.GetFirstAttribute(ref pageCurrent, ref idxCurrent))
		{
			pageParent = array;
			idxParent = num;
			return true;
		}
		return false;
	}

	public override bool MoveToNextAttribute()
	{
		return XPathNodeHelper.GetNextAttribute(ref pageCurrent, ref idxCurrent);
	}

	public override bool MoveToAttribute(string localName, string namespaceURI)
	{
		XPathNode[] array = pageCurrent;
		int num = idxCurrent;
		if ((object)localName != atomizedLocalName)
		{
			atomizedLocalName = ((localName != null) ? NameTable.Get(localName) : null);
		}
		if (XPathNodeHelper.GetAttribute(ref pageCurrent, ref idxCurrent, atomizedLocalName, namespaceURI))
		{
			pageParent = array;
			idxParent = num;
			return true;
		}
		return false;
	}

	public override bool MoveToFirstNamespace(XPathNamespaceScope namespaceScope)
	{
		XPathNode[] pageNmsp;
		for (int num = ((namespaceScope != XPathNamespaceScope.Local) ? XPathNodeHelper.GetInScopeNamespaces(pageCurrent, idxCurrent, out pageNmsp) : XPathNodeHelper.GetLocalNamespaces(pageCurrent, idxCurrent, out pageNmsp)); num != 0; num = pageNmsp[num].GetSibling(out pageNmsp))
		{
			if (namespaceScope != XPathNamespaceScope.ExcludeXml || !pageNmsp[num].IsXmlNamespaceNode)
			{
				pageParent = pageCurrent;
				idxParent = idxCurrent;
				pageCurrent = pageNmsp;
				idxCurrent = num;
				return true;
			}
		}
		return false;
	}

	public override bool MoveToNextNamespace(XPathNamespaceScope scope)
	{
		XPathNode[] pageNode = pageCurrent;
		int sibling = idxCurrent;
		if (pageNode[sibling].NodeType != XPathNodeType.Namespace)
		{
			return false;
		}
		do
		{
			sibling = pageNode[sibling].GetSibling(out pageNode);
			if (sibling == 0)
			{
				return false;
			}
			switch (scope)
			{
			case XPathNamespaceScope.Local:
			{
				if (pageNode[sibling].GetParent(out var pageNode2) != idxParent || pageNode2 != pageParent)
				{
					return false;
				}
				break;
			}
			case XPathNamespaceScope.ExcludeXml:
				continue;
			}
			break;
		}
		while (pageNode[sibling].IsXmlNamespaceNode);
		pageCurrent = pageNode;
		idxCurrent = sibling;
		return true;
	}

	public override bool MoveToNext()
	{
		return XPathNodeHelper.GetContentSibling(ref pageCurrent, ref idxCurrent);
	}

	public override bool MoveToPrevious()
	{
		if (idxParent != 0)
		{
			return false;
		}
		return XPathNodeHelper.GetPreviousContentSibling(ref pageCurrent, ref idxCurrent);
	}

	public override bool MoveToFirstChild()
	{
		if (pageCurrent[idxCurrent].HasCollapsedText)
		{
			pageParent = pageCurrent;
			idxParent = idxCurrent;
			idxCurrent = pageCurrent[idxCurrent].Document.GetCollapsedTextNode(out pageCurrent);
			return true;
		}
		return XPathNodeHelper.GetContentChild(ref pageCurrent, ref idxCurrent);
	}

	public override bool MoveToParent()
	{
		if (idxParent != 0)
		{
			pageCurrent = pageParent;
			idxCurrent = idxParent;
			pageParent = null;
			idxParent = 0;
			return true;
		}
		return XPathNodeHelper.GetParent(ref pageCurrent, ref idxCurrent);
	}

	public override bool MoveTo(XPathNavigator other)
	{
		if (other is XPathDocumentNavigator xPathDocumentNavigator)
		{
			pageCurrent = xPathDocumentNavigator.pageCurrent;
			idxCurrent = xPathDocumentNavigator.idxCurrent;
			pageParent = xPathDocumentNavigator.pageParent;
			idxParent = xPathDocumentNavigator.idxParent;
			return true;
		}
		return false;
	}

	public override bool MoveToId(string id)
	{
		XPathNode[] pageElem;
		int num = pageCurrent[idxCurrent].Document.LookupIdElement(id, out pageElem);
		if (num != 0)
		{
			pageCurrent = pageElem;
			idxCurrent = num;
			pageParent = null;
			idxParent = 0;
			return true;
		}
		return false;
	}

	public override bool IsSamePosition(XPathNavigator other)
	{
		if (other is XPathDocumentNavigator xPathDocumentNavigator)
		{
			if (idxCurrent == xPathDocumentNavigator.idxCurrent && pageCurrent == xPathDocumentNavigator.pageCurrent && idxParent == xPathDocumentNavigator.idxParent)
			{
				return pageParent == xPathDocumentNavigator.pageParent;
			}
			return false;
		}
		return false;
	}

	public override void MoveToRoot()
	{
		if (idxParent != 0)
		{
			pageParent = null;
			idxParent = 0;
		}
		idxCurrent = pageCurrent[idxCurrent].GetRoot(out pageCurrent);
	}

	public override bool MoveToChild(string localName, string namespaceURI)
	{
		if ((object)localName != atomizedLocalName)
		{
			atomizedLocalName = ((localName != null) ? NameTable.Get(localName) : null);
		}
		return XPathNodeHelper.GetElementChild(ref pageCurrent, ref idxCurrent, atomizedLocalName, namespaceURI);
	}

	public override bool MoveToNext(string localName, string namespaceURI)
	{
		if ((object)localName != atomizedLocalName)
		{
			atomizedLocalName = ((localName != null) ? NameTable.Get(localName) : null);
		}
		return XPathNodeHelper.GetElementSibling(ref pageCurrent, ref idxCurrent, atomizedLocalName, namespaceURI);
	}

	public override bool MoveToChild(XPathNodeType type)
	{
		if (pageCurrent[idxCurrent].HasCollapsedText)
		{
			if (type != XPathNodeType.Text && type != XPathNodeType.All)
			{
				return false;
			}
			pageParent = pageCurrent;
			idxParent = idxCurrent;
			idxCurrent = pageCurrent[idxCurrent].Document.GetCollapsedTextNode(out pageCurrent);
			return true;
		}
		return XPathNodeHelper.GetContentChild(ref pageCurrent, ref idxCurrent, type);
	}

	public override bool MoveToNext(XPathNodeType type)
	{
		return XPathNodeHelper.GetContentSibling(ref pageCurrent, ref idxCurrent, type);
	}

	public override bool MoveToFollowing(string localName, string namespaceURI, XPathNavigator end)
	{
		if ((object)localName != atomizedLocalName)
		{
			atomizedLocalName = ((localName != null) ? NameTable.Get(localName) : null);
		}
		XPathNode[] pageEnd;
		int followingEnd = GetFollowingEnd(end as XPathDocumentNavigator, useParentOfVirtual: false, out pageEnd);
		if (idxParent != 0)
		{
			if (!XPathNodeHelper.GetElementFollowing(ref pageParent, ref idxParent, pageEnd, followingEnd, atomizedLocalName, namespaceURI))
			{
				return false;
			}
			pageCurrent = pageParent;
			idxCurrent = idxParent;
			pageParent = null;
			idxParent = 0;
			return true;
		}
		return XPathNodeHelper.GetElementFollowing(ref pageCurrent, ref idxCurrent, pageEnd, followingEnd, atomizedLocalName, namespaceURI);
	}

	public override bool MoveToFollowing(XPathNodeType type, XPathNavigator end)
	{
		XPathDocumentNavigator xPathDocumentNavigator = end as XPathDocumentNavigator;
		XPathNode[] pageEnd;
		int followingEnd;
		if (type == XPathNodeType.Text || type == XPathNodeType.All)
		{
			if (pageCurrent[idxCurrent].HasCollapsedText)
			{
				if (xPathDocumentNavigator != null && idxCurrent == xPathDocumentNavigator.idxParent && pageCurrent == xPathDocumentNavigator.pageParent)
				{
					return false;
				}
				pageParent = pageCurrent;
				idxParent = idxCurrent;
				idxCurrent = pageCurrent[idxCurrent].Document.GetCollapsedTextNode(out pageCurrent);
				return true;
			}
			if (type == XPathNodeType.Text)
			{
				followingEnd = GetFollowingEnd(xPathDocumentNavigator, useParentOfVirtual: true, out pageEnd);
				XPathNode[] array;
				int num;
				if (idxParent != 0)
				{
					array = pageParent;
					num = idxParent;
				}
				else
				{
					array = pageCurrent;
					num = idxCurrent;
				}
				if (xPathDocumentNavigator != null && xPathDocumentNavigator.idxParent != 0 && num == followingEnd && array == pageEnd)
				{
					return false;
				}
				if (!XPathNodeHelper.GetTextFollowing(ref array, ref num, pageEnd, followingEnd))
				{
					return false;
				}
				if (array[num].NodeType == XPathNodeType.Element)
				{
					idxCurrent = array[num].Document.GetCollapsedTextNode(out pageCurrent);
					pageParent = array;
					idxParent = num;
				}
				else
				{
					pageCurrent = array;
					idxCurrent = num;
					pageParent = null;
					idxParent = 0;
				}
				return true;
			}
		}
		followingEnd = GetFollowingEnd(xPathDocumentNavigator, useParentOfVirtual: false, out pageEnd);
		if (idxParent != 0)
		{
			if (!XPathNodeHelper.GetContentFollowing(ref pageParent, ref idxParent, pageEnd, followingEnd, type))
			{
				return false;
			}
			pageCurrent = pageParent;
			idxCurrent = idxParent;
			pageParent = null;
			idxParent = 0;
			return true;
		}
		return XPathNodeHelper.GetContentFollowing(ref pageCurrent, ref idxCurrent, pageEnd, followingEnd, type);
	}

	public override XPathNodeIterator SelectChildren(XPathNodeType type)
	{
		return new XPathDocumentKindChildIterator(this, type);
	}

	public override XPathNodeIterator SelectChildren(string name, string namespaceURI)
	{
		if (name == null || name.Length == 0)
		{
			return base.SelectChildren(name, namespaceURI);
		}
		return new XPathDocumentElementChildIterator(this, name, namespaceURI);
	}

	public override XPathNodeIterator SelectDescendants(XPathNodeType type, bool matchSelf)
	{
		return new XPathDocumentKindDescendantIterator(this, type, matchSelf);
	}

	public override XPathNodeIterator SelectDescendants(string name, string namespaceURI, bool matchSelf)
	{
		if (name == null || name.Length == 0)
		{
			return base.SelectDescendants(name, namespaceURI, matchSelf);
		}
		return new XPathDocumentElementDescendantIterator(this, name, namespaceURI, matchSelf);
	}

	public override XmlNodeOrder ComparePosition(XPathNavigator other)
	{
		if (other is XPathDocumentNavigator xPathDocumentNavigator)
		{
			XPathDocument document = pageCurrent[idxCurrent].Document;
			XPathDocument document2 = xPathDocumentNavigator.pageCurrent[xPathDocumentNavigator.idxCurrent].Document;
			if (document == document2)
			{
				int num = GetPrimaryLocation();
				int num2 = xPathDocumentNavigator.GetPrimaryLocation();
				if (num == num2)
				{
					num = GetSecondaryLocation();
					num2 = xPathDocumentNavigator.GetSecondaryLocation();
					if (num == num2)
					{
						return XmlNodeOrder.Same;
					}
				}
				if (num >= num2)
				{
					return XmlNodeOrder.After;
				}
				return XmlNodeOrder.Before;
			}
		}
		return XmlNodeOrder.Unknown;
	}

	public override bool IsDescendant(XPathNavigator other)
	{
		if (other is XPathDocumentNavigator xPathDocumentNavigator)
		{
			XPathNode[] pageNode;
			int parent;
			if (xPathDocumentNavigator.idxParent != 0)
			{
				pageNode = xPathDocumentNavigator.pageParent;
				parent = xPathDocumentNavigator.idxParent;
			}
			else
			{
				parent = xPathDocumentNavigator.pageCurrent[xPathDocumentNavigator.idxCurrent].GetParent(out pageNode);
			}
			while (parent != 0)
			{
				if (parent == idxCurrent && pageNode == pageCurrent)
				{
					return true;
				}
				parent = pageNode[parent].GetParent(out pageNode);
			}
		}
		return false;
	}

	private int GetPrimaryLocation()
	{
		if (idxParent == 0)
		{
			return XPathNodeHelper.GetLocation(pageCurrent, idxCurrent);
		}
		return XPathNodeHelper.GetLocation(pageParent, idxParent);
	}

	private int GetSecondaryLocation()
	{
		if (idxParent == 0)
		{
			return int.MinValue;
		}
		return pageCurrent[idxCurrent].NodeType switch
		{
			XPathNodeType.Namespace => -2147483647 + XPathNodeHelper.GetLocation(pageCurrent, idxCurrent), 
			XPathNodeType.Attribute => XPathNodeHelper.GetLocation(pageCurrent, idxCurrent), 
			_ => int.MaxValue, 
		};
	}

	public bool HasLineInfo()
	{
		return pageCurrent[idxCurrent].Document.HasLineInfo;
	}

	public int GetPositionHashCode()
	{
		return idxCurrent ^ idxParent;
	}

	public bool IsElementMatch(string localName, string namespaceURI)
	{
		if ((object)localName != atomizedLocalName)
		{
			atomizedLocalName = ((localName != null) ? NameTable.Get(localName) : null);
		}
		if (idxParent != 0)
		{
			return false;
		}
		return pageCurrent[idxCurrent].ElementMatch(atomizedLocalName, namespaceURI);
	}

	public bool IsContentKindMatch(XPathNodeType typ)
	{
		return ((1 << (int)pageCurrent[idxCurrent].NodeType) & XPathNavigator.GetContentKindMask(typ)) != 0;
	}

	public bool IsKindMatch(XPathNodeType typ)
	{
		return ((1 << (int)pageCurrent[idxCurrent].NodeType) & XPathNavigator.GetKindMask(typ)) != 0;
	}

	private int GetFollowingEnd(XPathDocumentNavigator end, bool useParentOfVirtual, out XPathNode[] pageEnd)
	{
		if (end != null && pageCurrent[idxCurrent].Document == end.pageCurrent[end.idxCurrent].Document)
		{
			if (end.idxParent == 0)
			{
				pageEnd = end.pageCurrent;
				return end.idxCurrent;
			}
			pageEnd = end.pageParent;
			if (!useParentOfVirtual)
			{
				return end.idxParent + 1;
			}
			return end.idxParent;
		}
		pageEnd = null;
		return 0;
	}
}
