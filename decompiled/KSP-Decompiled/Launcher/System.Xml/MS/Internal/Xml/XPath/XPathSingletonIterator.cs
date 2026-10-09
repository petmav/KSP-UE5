using System.Xml.XPath;

namespace MS.Internal.Xml.XPath;

internal class XPathSingletonIterator : ResetableIterator
{
	private XPathNavigator nav;

	private int position;

	public override XPathNavigator Current => nav;

	public override int CurrentPosition => position;

	public override int Count => 1;

	public XPathSingletonIterator(XPathNavigator nav)
	{
		this.nav = nav;
	}

	public XPathSingletonIterator(XPathNavigator nav, bool moved)
		: this(nav)
	{
		if (moved)
		{
			position = 1;
		}
	}

	public XPathSingletonIterator(XPathSingletonIterator it)
	{
		nav = it.nav.Clone();
		position = it.position;
	}

	public override XPathNodeIterator Clone()
	{
		return new XPathSingletonIterator(this);
	}

	public override bool MoveNext()
	{
		if (position == 0)
		{
			position = 1;
			return true;
		}
		return false;
	}

	public override void Reset()
	{
		position = 0;
	}
}
