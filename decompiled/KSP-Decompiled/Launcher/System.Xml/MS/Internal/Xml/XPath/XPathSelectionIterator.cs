using System.Xml.XPath;

namespace MS.Internal.Xml.XPath;

internal class XPathSelectionIterator : ResetableIterator
{
	private XPathNavigator nav;

	private Query query;

	private int position;

	public override int Count => query.Count;

	public override XPathNavigator Current => nav;

	public override int CurrentPosition => position;

	internal XPathSelectionIterator(XPathNavigator nav, Query query)
	{
		this.nav = nav.Clone();
		this.query = query;
	}

	protected XPathSelectionIterator(XPathSelectionIterator it)
	{
		nav = it.nav.Clone();
		query = (Query)it.query.Clone();
		position = it.position;
	}

	public override void Reset()
	{
		query.Reset();
	}

	public override bool MoveNext()
	{
		XPathNavigator xPathNavigator = query.Advance();
		if (xPathNavigator != null)
		{
			position++;
			if (!nav.MoveTo(xPathNavigator))
			{
				nav = xPathNavigator.Clone();
			}
			return true;
		}
		return false;
	}

	public override XPathNodeIterator Clone()
	{
		return new XPathSelectionIterator(this);
	}
}
