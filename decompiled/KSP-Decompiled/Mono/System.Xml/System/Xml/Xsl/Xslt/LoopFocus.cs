using System.Xml.Xsl.Qil;
using System.Xml.Xsl.XPath;

namespace System.Xml.Xsl.Xslt;

internal struct LoopFocus : IFocus
{
	private XPathQilFactory f;

	private QilIterator current;

	private QilIterator cached;

	private QilIterator last;

	public bool IsFocusSet => current != null;

	public LoopFocus(XPathQilFactory f)
	{
		this.f = f;
		current = (cached = (last = null));
	}

	public void SetFocus(QilIterator current)
	{
		this.current = current;
		cached = (last = null);
	}

	public QilNode GetCurrent()
	{
		return current;
	}

	public QilNode GetPosition()
	{
		return f.XsltConvert(f.PositionOf(current), XmlQueryTypeFactory.DoubleX);
	}

	public QilNode GetLast()
	{
		if (last == null)
		{
			last = f.Let(f.Double(0.0));
		}
		return last;
	}

	public void EnsureCache()
	{
		if (cached == null)
		{
			cached = f.Let(current.Binding);
			current.Binding = cached;
		}
	}

	public void Sort(QilNode sortKeys)
	{
		if (sortKeys != null)
		{
			EnsureCache();
			current = f.For(f.Sort(current, sortKeys));
		}
	}

	public QilLoop ConstructLoop(QilNode body)
	{
		if (last != null)
		{
			EnsureCache();
			last.Binding = f.XsltConvert(f.Length(cached), XmlQueryTypeFactory.DoubleX);
		}
		QilLoop qilLoop = f.BaseFactory.Loop(current, body);
		if (last != null)
		{
			qilLoop = f.BaseFactory.Loop(last, qilLoop);
		}
		if (cached != null)
		{
			qilLoop = f.BaseFactory.Loop(cached, qilLoop);
		}
		return qilLoop;
	}
}
