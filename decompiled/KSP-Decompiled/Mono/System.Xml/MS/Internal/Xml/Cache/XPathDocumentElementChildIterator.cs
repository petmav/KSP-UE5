using System;
using System.Xml.XPath;

namespace MS.Internal.Xml.Cache;

internal class XPathDocumentElementChildIterator : XPathDocumentBaseIterator
{
	private string localName;

	private string namespaceUri;

	public XPathDocumentElementChildIterator(XPathDocumentNavigator parent, string name, string namespaceURI)
		: base(parent)
	{
		if (namespaceURI == null)
		{
			throw new ArgumentNullException("namespaceURI");
		}
		localName = parent.NameTable.Get(name);
		namespaceUri = namespaceURI;
	}

	public XPathDocumentElementChildIterator(XPathDocumentElementChildIterator iter)
		: base(iter)
	{
		localName = iter.localName;
		namespaceUri = iter.namespaceUri;
	}

	public override XPathNodeIterator Clone()
	{
		return new XPathDocumentElementChildIterator(this);
	}

	public override bool MoveNext()
	{
		if (pos == 0)
		{
			if (!ctxt.MoveToChild(localName, namespaceUri))
			{
				return false;
			}
		}
		else if (!ctxt.MoveToNext(localName, namespaceUri))
		{
			return false;
		}
		pos++;
		return true;
	}
}
