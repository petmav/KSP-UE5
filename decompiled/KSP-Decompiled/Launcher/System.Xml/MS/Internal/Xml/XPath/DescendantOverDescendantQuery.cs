using System.Xml.XPath;

namespace MS.Internal.Xml.XPath;

internal sealed class DescendantOverDescendantQuery : DescendantBaseQuery
{
	private int level;

	public DescendantOverDescendantQuery(Query qyParent, bool matchSelf, string name, string prefix, XPathNodeType typeTest, bool abbrAxis)
		: base(qyParent, name, prefix, typeTest, matchSelf, abbrAxis)
	{
	}

	private DescendantOverDescendantQuery(DescendantOverDescendantQuery other)
		: base(other)
	{
		level = other.level;
	}

	public override void Reset()
	{
		level = 0;
		base.Reset();
	}

	public override XPathNavigator Advance()
	{
		while (true)
		{
			if (level == 0)
			{
				currentNode = qyInput.Advance();
				position = 0;
				if (currentNode == null)
				{
					return null;
				}
				if (matchSelf && matches(currentNode))
				{
					break;
				}
				currentNode = currentNode.Clone();
				if (!MoveToFirstChild())
				{
					continue;
				}
			}
			else if (!MoveUpUntillNext())
			{
				continue;
			}
			do
			{
				if (matches(currentNode))
				{
					position++;
					return currentNode;
				}
			}
			while (MoveToFirstChild());
		}
		position = 1;
		return currentNode;
	}

	private bool MoveToFirstChild()
	{
		if (currentNode.MoveToFirstChild())
		{
			level++;
			return true;
		}
		return false;
	}

	private bool MoveUpUntillNext()
	{
		while (!currentNode.MoveToNext())
		{
			level--;
			if (level == 0)
			{
				return false;
			}
			currentNode.MoveToParent();
		}
		return true;
	}

	public override XPathNodeIterator Clone()
	{
		return new DescendantOverDescendantQuery(this);
	}
}
