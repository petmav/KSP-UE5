using System.Xml.XPath;

namespace MS.Internal.Xml.XPath;

internal sealed class NamespaceQuery : BaseAxisQuery
{
	private bool onNamespace;

	public NamespaceQuery(Query qyParent, string Name, string Prefix, XPathNodeType Type)
		: base(qyParent, Name, Prefix, Type)
	{
	}

	private NamespaceQuery(NamespaceQuery other)
		: base(other)
	{
		onNamespace = other.onNamespace;
	}

	public override void Reset()
	{
		onNamespace = false;
		base.Reset();
	}

	public override XPathNavigator Advance()
	{
		do
		{
			if (!onNamespace)
			{
				currentNode = qyInput.Advance();
				if (currentNode == null)
				{
					return null;
				}
				position = 0;
				currentNode = currentNode.Clone();
				onNamespace = currentNode.MoveToFirstNamespace();
			}
			else
			{
				onNamespace = currentNode.MoveToNextNamespace();
			}
		}
		while (!onNamespace || !matches(currentNode));
		position++;
		return currentNode;
	}

	public override bool matches(XPathNavigator e)
	{
		if (e.Value.Length == 0)
		{
			return false;
		}
		if (base.NameTest)
		{
			return base.Name.Equals(e.LocalName);
		}
		return true;
	}

	public override XPathNodeIterator Clone()
	{
		return new NamespaceQuery(this);
	}
}
