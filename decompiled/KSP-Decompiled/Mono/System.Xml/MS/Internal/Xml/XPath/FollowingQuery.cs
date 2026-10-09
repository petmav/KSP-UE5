using System.Xml.XPath;

namespace MS.Internal.Xml.XPath;

internal sealed class FollowingQuery : BaseAxisQuery
{
	private XPathNavigator input;

	private XPathNodeIterator iterator;

	public FollowingQuery(Query qyInput, string name, string prefix, XPathNodeType typeTest)
		: base(qyInput, name, prefix, typeTest)
	{
	}

	private FollowingQuery(FollowingQuery other)
		: base(other)
	{
		input = Query.Clone(other.input);
		iterator = Query.Clone(other.iterator);
	}

	public override void Reset()
	{
		iterator = null;
		base.Reset();
	}

	public override XPathNavigator Advance()
	{
		if (iterator == null)
		{
			input = qyInput.Advance();
			if (input == null)
			{
				return null;
			}
			XPathNavigator xPathNavigator;
			do
			{
				xPathNavigator = input.Clone();
				input = qyInput.Advance();
			}
			while (xPathNavigator.IsDescendant(input));
			input = xPathNavigator;
			iterator = XPathEmptyIterator.Instance;
		}
		while (!iterator.MoveNext())
		{
			bool matchSelf;
			if (input.NodeType == XPathNodeType.Attribute || input.NodeType == XPathNodeType.Namespace)
			{
				input.MoveToParent();
				matchSelf = false;
			}
			else
			{
				while (!input.MoveToNext())
				{
					if (!input.MoveToParent())
					{
						return null;
					}
				}
				matchSelf = true;
			}
			if (base.NameTest)
			{
				iterator = input.SelectDescendants(base.Name, base.Namespace, matchSelf);
			}
			else
			{
				iterator = input.SelectDescendants(base.TypeTest, matchSelf);
			}
		}
		position++;
		currentNode = iterator.Current;
		return currentNode;
	}

	public override XPathNodeIterator Clone()
	{
		return new FollowingQuery(this);
	}
}
