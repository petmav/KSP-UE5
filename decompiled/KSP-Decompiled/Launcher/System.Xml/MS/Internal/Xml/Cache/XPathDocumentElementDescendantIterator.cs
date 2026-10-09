using System;
using System.Xml.XPath;

namespace MS.Internal.Xml.Cache;

internal class XPathDocumentElementDescendantIterator : XPathDocumentBaseIterator
{
	private XPathDocumentNavigator end;

	private string localName;

	private string namespaceUri;

	private bool matchSelf;

	public XPathDocumentElementDescendantIterator(XPathDocumentNavigator root, string name, string namespaceURI, bool matchSelf)
		: base(root)
	{
		if (namespaceURI == null)
		{
			throw new ArgumentNullException("namespaceURI");
		}
		localName = root.NameTable.Get(name);
		namespaceUri = namespaceURI;
		this.matchSelf = matchSelf;
		if (root.NodeType != 0)
		{
			end = new XPathDocumentNavigator(root);
			end.MoveToNonDescendant();
		}
	}

	public XPathDocumentElementDescendantIterator(XPathDocumentElementDescendantIterator iter)
		: base(iter)
	{
		end = iter.end;
		localName = iter.localName;
		namespaceUri = iter.namespaceUri;
		matchSelf = iter.matchSelf;
	}

	public override XPathNodeIterator Clone()
	{
		return new XPathDocumentElementDescendantIterator(this);
	}

	public override bool MoveNext()
	{
		if (matchSelf)
		{
			matchSelf = false;
			if (ctxt.IsElementMatch(localName, namespaceUri))
			{
				pos++;
				return true;
			}
		}
		if (!ctxt.MoveToFollowing(localName, namespaceUri, end))
		{
			return false;
		}
		pos++;
		return true;
	}
}
