using System.Collections;

namespace System.Xml.Schema;

internal class ActiveAxis
{
	private int currentDepth;

	private bool isActive;

	private Asttree axisTree;

	private ArrayList axisStack;

	public int CurrentDepth => currentDepth;

	internal void Reactivate()
	{
		isActive = true;
		currentDepth = -1;
	}

	internal ActiveAxis(Asttree axisTree)
	{
		this.axisTree = axisTree;
		currentDepth = -1;
		axisStack = new ArrayList(axisTree.SubtreeArray.Count);
		for (int i = 0; i < axisTree.SubtreeArray.Count; i++)
		{
			AxisStack value = new AxisStack((ForwardAxis)axisTree.SubtreeArray[i], this);
			axisStack.Add(value);
		}
		isActive = true;
	}

	public bool MoveToStartElement(string localname, string URN)
	{
		if (!isActive)
		{
			return false;
		}
		currentDepth++;
		bool result = false;
		for (int i = 0; i < this.axisStack.Count; i++)
		{
			AxisStack axisStack = (AxisStack)this.axisStack[i];
			if (axisStack.Subtree.IsSelfAxis)
			{
				if (axisStack.Subtree.IsDss || CurrentDepth == 0)
				{
					result = true;
				}
			}
			else if (CurrentDepth != 0 && axisStack.MoveToChild(localname, URN, currentDepth))
			{
				result = true;
			}
		}
		return result;
	}

	public virtual bool EndElement(string localname, string URN)
	{
		if (currentDepth == 0)
		{
			isActive = false;
			currentDepth--;
		}
		if (!isActive)
		{
			return false;
		}
		for (int i = 0; i < axisStack.Count; i++)
		{
			((AxisStack)axisStack[i]).MoveToParent(localname, URN, currentDepth);
		}
		currentDepth--;
		return false;
	}

	public bool MoveToAttribute(string localname, string URN)
	{
		if (!isActive)
		{
			return false;
		}
		bool result = false;
		for (int i = 0; i < axisStack.Count; i++)
		{
			if (((AxisStack)axisStack[i]).MoveToAttribute(localname, URN, currentDepth + 1))
			{
				result = true;
			}
		}
		return result;
	}
}
