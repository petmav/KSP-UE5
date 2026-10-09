using System.Xml.XPath;

namespace MS.Internal.Xml.XPath;

internal sealed class AttributeQuery : BaseAxisQuery
{
	private bool onAttribute;

	public AttributeQuery(Query qyParent, string Name, string Prefix, XPathNodeType Type)
		: base(qyParent, Name, Prefix, Type)
	{
	}

	private AttributeQuery(AttributeQuery other)
		: base(other)
	{
		onAttribute = other.onAttribute;
	}

	public override void Reset()
	{
		onAttribute = false;
		base.Reset();
	}

	public override XPathNavigator Advance()
	{
		do
		{
			if (!onAttribute)
			{
				currentNode = qyInput.Advance();
				if (currentNode == null)
				{
					return null;
				}
				position = 0;
				currentNode = currentNode.Clone();
				onAttribute = currentNode.MoveToFirstAttribute();
			}
			else
			{
				onAttribute = currentNode.MoveToNextAttribute();
			}
		}
		while (!onAttribute || !matches(currentNode));
		position++;
		return currentNode;
	}

	public override XPathNavigator MatchNode(XPathNavigator context)
	{
		if (context != null && context.NodeType == XPathNodeType.Attribute && matches(context))
		{
			XPathNavigator xPathNavigator = context.Clone();
			if (xPathNavigator.MoveToParent())
			{
				return qyInput.MatchNode(xPathNavigator);
			}
		}
		return null;
	}

	public override XPathNodeIterator Clone()
	{
		return new AttributeQuery(this);
	}
}
