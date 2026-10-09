using System.Xml.XPath;

namespace MS.Internal.Xml.Cache;

internal class XPathDocumentKindDescendantIterator : XPathDocumentBaseIterator
{
	private XPathDocumentNavigator end;

	private XPathNodeType typ;

	private bool matchSelf;

	public XPathDocumentKindDescendantIterator(XPathDocumentNavigator root, XPathNodeType typ, bool matchSelf)
		: base(root)
	{
		this.typ = typ;
		this.matchSelf = matchSelf;
		if (root.NodeType != 0)
		{
			end = new XPathDocumentNavigator(root);
			end.MoveToNonDescendant();
		}
	}

	public XPathDocumentKindDescendantIterator(XPathDocumentKindDescendantIterator iter)
		: base(iter)
	{
		end = iter.end;
		typ = iter.typ;
		matchSelf = iter.matchSelf;
	}

	public override XPathNodeIterator Clone()
	{
		return new XPathDocumentKindDescendantIterator(this);
	}

	public override bool MoveNext()
	{
		if (matchSelf)
		{
			matchSelf = false;
			if (ctxt.IsKindMatch(typ))
			{
				pos++;
				return true;
			}
		}
		if (!ctxt.MoveToFollowing(typ, end))
		{
			return false;
		}
		pos++;
		return true;
	}
}
