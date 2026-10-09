using System.Collections;

namespace System.Xml.Linq;

internal struct Inserter
{
	private XContainer parent;

	private XNode previous;

	private string text;

	public Inserter(XContainer parent, XNode anchor)
	{
		this.parent = parent;
		previous = anchor;
		text = null;
	}

	public void Add(object content)
	{
		AddContent(content);
		if (text == null)
		{
			return;
		}
		if (parent.content == null)
		{
			if (parent.SkipNotify())
			{
				parent.content = text;
			}
			else if (text.Length > 0)
			{
				InsertNode(new XText(text));
			}
			else if (parent is XElement)
			{
				parent.NotifyChanging(parent, XObjectChangeEventArgs.Value);
				if (parent.content != null)
				{
					throw new InvalidOperationException(Res.GetString("InvalidOperation_ExternalCode"));
				}
				parent.content = text;
				parent.NotifyChanged(parent, XObjectChangeEventArgs.Value);
			}
			else
			{
				parent.content = text;
			}
		}
		else if (text.Length > 0)
		{
			if (previous is XText && !(previous is XCData))
			{
				((XText)previous).Value += text;
				return;
			}
			parent.ConvertTextToNode();
			InsertNode(new XText(text));
		}
	}

	private void AddContent(object content)
	{
		if (content == null)
		{
			return;
		}
		if (content is XNode n)
		{
			AddNode(n);
			return;
		}
		if (content is string s)
		{
			AddString(s);
			return;
		}
		if (content is XStreamingElement other)
		{
			AddNode(new XElement(other));
			return;
		}
		if (content is object[] array)
		{
			object[] array2 = array;
			foreach (object content2 in array2)
			{
				AddContent(content2);
			}
			return;
		}
		if (content is IEnumerable enumerable)
		{
			{
				foreach (object item in enumerable)
				{
					AddContent(item);
				}
				return;
			}
		}
		if (content is XAttribute)
		{
			throw new ArgumentException(Res.GetString("Argument_AddAttribute"));
		}
		AddString(XContainer.GetStringValue(content));
	}

	private void AddNode(XNode n)
	{
		parent.ValidateNode(n, previous);
		if (n.parent != null)
		{
			n = n.CloneNode();
		}
		else
		{
			XNode xNode = parent;
			while (xNode.parent != null)
			{
				xNode = xNode.parent;
			}
			if (n == xNode)
			{
				n = n.CloneNode();
			}
		}
		parent.ConvertTextToNode();
		if (text != null)
		{
			if (text.Length > 0)
			{
				if (previous is XText && !(previous is XCData))
				{
					((XText)previous).Value += text;
				}
				else
				{
					InsertNode(new XText(text));
				}
			}
			text = null;
		}
		InsertNode(n);
	}

	private void AddString(string s)
	{
		parent.ValidateString(s);
		text += s;
	}

	private void InsertNode(XNode n)
	{
		bool num = parent.NotifyChanging(n, XObjectChangeEventArgs.Add);
		if (n.parent != null)
		{
			throw new InvalidOperationException(Res.GetString("InvalidOperation_ExternalCode"));
		}
		n.parent = parent;
		if (parent.content == null || parent.content is string)
		{
			n.next = n;
			parent.content = n;
		}
		else if (previous == null)
		{
			XNode xNode = (XNode)parent.content;
			n.next = xNode.next;
			xNode.next = n;
		}
		else
		{
			n.next = previous.next;
			previous.next = n;
			if (parent.content == previous)
			{
				parent.content = n;
			}
		}
		previous = n;
		if (num)
		{
			parent.NotifyChanged(n, XObjectChangeEventArgs.Add);
		}
	}
}
