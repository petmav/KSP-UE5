using System.Xml;
using System.Xml.XPath;

namespace MS.Internal.Xml.XPath;

internal sealed class XPathAncestorQuery : CacheAxisQuery
{
	private bool matchSelf;

	public override int CurrentPosition => outputBuffer.Count - count + 1;

	public override QueryProps Properties => base.Properties | QueryProps.Reverse;

	public XPathAncestorQuery(Query qyInput, string name, string prefix, XPathNodeType typeTest, bool matchSelf)
		: base(qyInput, name, prefix, typeTest)
	{
		this.matchSelf = matchSelf;
	}

	private XPathAncestorQuery(XPathAncestorQuery other)
		: base(other)
	{
		matchSelf = other.matchSelf;
	}

	public override object Evaluate(XPathNodeIterator context)
	{
		base.Evaluate(context);
		XPathNavigator xPathNavigator = null;
		XPathNavigator xPathNavigator2;
		while ((xPathNavigator2 = qyInput.Advance()) != null)
		{
			if (!matchSelf || !matches(xPathNavigator2) || Insert(outputBuffer, xPathNavigator2))
			{
				if (xPathNavigator == null || !xPathNavigator.MoveTo(xPathNavigator2))
				{
					xPathNavigator = xPathNavigator2.Clone();
				}
				while (xPathNavigator.MoveToParent() && (!matches(xPathNavigator) || Insert(outputBuffer, xPathNavigator)))
				{
				}
			}
		}
		return this;
	}

	public override XPathNodeIterator Clone()
	{
		return new XPathAncestorQuery(this);
	}

	public override void PrintQuery(XmlWriter w)
	{
		w.WriteStartElement(GetType().Name);
		if (matchSelf)
		{
			w.WriteAttributeString("self", "yes");
		}
		if (base.NameTest)
		{
			w.WriteAttributeString("name", (base.Prefix.Length != 0) ? (base.Prefix + ":" + base.Name) : base.Name);
		}
		if (base.TypeTest != XPathNodeType.Element)
		{
			w.WriteAttributeString("nodeType", base.TypeTest.ToString());
		}
		qyInput.PrintQuery(w);
		w.WriteEndElement();
	}
}
