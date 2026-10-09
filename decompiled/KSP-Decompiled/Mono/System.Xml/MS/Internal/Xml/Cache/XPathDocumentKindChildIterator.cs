using System.Xml.XPath;

namespace MS.Internal.Xml.Cache;

internal class XPathDocumentKindChildIterator : XPathDocumentBaseIterator
{
	private XPathNodeType typ;

	public XPathDocumentKindChildIterator(XPathDocumentNavigator parent, XPathNodeType typ)
		: base(parent)
	{
		this.typ = typ;
	}

	public XPathDocumentKindChildIterator(XPathDocumentKindChildIterator iter)
		: base(iter)
	{
		typ = iter.typ;
	}

	public override XPathNodeIterator Clone()
	{
		return new XPathDocumentKindChildIterator(this);
	}

	public override bool MoveNext()
	{
		if (pos == 0)
		{
			if (!ctxt.MoveToChild(typ))
			{
				return false;
			}
		}
		else if (!ctxt.MoveToNext(typ))
		{
			return false;
		}
		pos++;
		return true;
	}
}
