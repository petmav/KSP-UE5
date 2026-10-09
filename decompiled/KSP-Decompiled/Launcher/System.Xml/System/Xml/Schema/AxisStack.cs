using System.Collections;

namespace System.Xml.Schema;

internal class AxisStack
{
	private ArrayList stack;

	private ForwardAxis subtree;

	private ActiveAxis parent;

	internal ForwardAxis Subtree => subtree;

	internal int Length => stack.Count;

	public AxisStack(ForwardAxis faxis, ActiveAxis parent)
	{
		subtree = faxis;
		stack = new ArrayList();
		this.parent = parent;
		if (!faxis.IsDss)
		{
			Push(1);
		}
	}

	internal void Push(int depth)
	{
		AxisElement value = new AxisElement(subtree.RootNode, depth);
		stack.Add(value);
	}

	internal void Pop()
	{
		stack.RemoveAt(Length - 1);
	}

	internal static bool Equal(string thisname, string thisURN, string name, string URN)
	{
		if (thisURN == null)
		{
			if (URN != null && URN.Length != 0)
			{
				return false;
			}
		}
		else if (thisURN.Length != 0 && thisURN != URN)
		{
			return false;
		}
		if (thisname.Length != 0 && thisname != name)
		{
			return false;
		}
		return true;
	}

	internal void MoveToParent(string name, string URN, int depth)
	{
		if (!subtree.IsSelfAxis)
		{
			for (int i = 0; i < stack.Count; i++)
			{
				((AxisElement)stack[i]).MoveToParent(depth, subtree);
			}
			if (subtree.IsDss && Equal(subtree.RootNode.Name, subtree.RootNode.Urn, name, URN))
			{
				Pop();
			}
		}
	}

	internal bool MoveToChild(string name, string URN, int depth)
	{
		bool result = false;
		if (subtree.IsDss && Equal(subtree.RootNode.Name, subtree.RootNode.Urn, name, URN))
		{
			Push(-1);
		}
		for (int i = 0; i < stack.Count; i++)
		{
			if (((AxisElement)stack[i]).MoveToChild(name, URN, depth, subtree))
			{
				result = true;
			}
		}
		return result;
	}

	internal bool MoveToAttribute(string name, string URN, int depth)
	{
		if (!subtree.IsAttribute)
		{
			return false;
		}
		if (!Equal(subtree.TopNode.Name, subtree.TopNode.Urn, name, URN))
		{
			return false;
		}
		bool result = false;
		if (subtree.TopNode.Input == null)
		{
			if (!subtree.IsDss)
			{
				return depth == 1;
			}
			return true;
		}
		for (int i = 0; i < stack.Count; i++)
		{
			AxisElement axisElement = (AxisElement)stack[i];
			if (axisElement.isMatch && axisElement.CurNode == subtree.TopNode.Input)
			{
				result = true;
			}
		}
		return result;
	}
}
