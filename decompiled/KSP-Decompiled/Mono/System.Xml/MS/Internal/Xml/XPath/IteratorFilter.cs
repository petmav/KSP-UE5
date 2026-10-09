using System.Xml.XPath;

namespace MS.Internal.Xml.XPath;

internal class IteratorFilter : XPathNodeIterator
{
	private XPathNodeIterator innerIterator;

	private string name;

	private int position;

	public override XPathNavigator Current => innerIterator.Current;

	public override int CurrentPosition => position;

	internal IteratorFilter(XPathNodeIterator innerIterator, string name)
	{
		this.innerIterator = innerIterator;
		this.name = name;
	}

	private IteratorFilter(IteratorFilter it)
	{
		innerIterator = it.innerIterator.Clone();
		name = it.name;
		position = it.position;
	}

	public override XPathNodeIterator Clone()
	{
		return new IteratorFilter(this);
	}

	public override bool MoveNext()
	{
		while (innerIterator.MoveNext())
		{
			if (innerIterator.Current.LocalName == name)
			{
				position++;
				return true;
			}
		}
		return false;
	}
}
