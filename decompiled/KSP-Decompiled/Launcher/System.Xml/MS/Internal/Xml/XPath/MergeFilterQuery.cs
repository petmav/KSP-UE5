using System.Xml;
using System.Xml.XPath;
using System.Xml.Xsl;

namespace MS.Internal.Xml.XPath;

internal sealed class MergeFilterQuery : CacheOutputQuery
{
	private Query child;

	public MergeFilterQuery(Query input, Query child)
		: base(input)
	{
		this.child = child;
	}

	private MergeFilterQuery(MergeFilterQuery other)
		: base(other)
	{
		child = Query.Clone(other.child);
	}

	public override void SetXsltContext(XsltContext xsltContext)
	{
		base.SetXsltContext(xsltContext);
		child.SetXsltContext(xsltContext);
	}

	public override object Evaluate(XPathNodeIterator nodeIterator)
	{
		base.Evaluate(nodeIterator);
		while (input.Advance() != null)
		{
			child.Evaluate(input);
			XPathNavigator nav;
			while ((nav = child.Advance()) != null)
			{
				Insert(outputBuffer, nav);
			}
		}
		return this;
	}

	public override XPathNavigator MatchNode(XPathNavigator current)
	{
		XPathNavigator xPathNavigator = child.MatchNode(current);
		if (xPathNavigator == null)
		{
			return null;
		}
		xPathNavigator = input.MatchNode(xPathNavigator);
		if (xPathNavigator == null)
		{
			return null;
		}
		Evaluate(new XPathSingletonIterator(xPathNavigator.Clone(), moved: true));
		for (XPathNavigator xPathNavigator2 = Advance(); xPathNavigator2 != null; xPathNavigator2 = Advance())
		{
			if (xPathNavigator2.IsSamePosition(current))
			{
				return xPathNavigator;
			}
		}
		return null;
	}

	public override XPathNodeIterator Clone()
	{
		return new MergeFilterQuery(this);
	}

	public override void PrintQuery(XmlWriter w)
	{
		w.WriteStartElement(GetType().Name);
		input.PrintQuery(w);
		child.PrintQuery(w);
		w.WriteEndElement();
	}
}
