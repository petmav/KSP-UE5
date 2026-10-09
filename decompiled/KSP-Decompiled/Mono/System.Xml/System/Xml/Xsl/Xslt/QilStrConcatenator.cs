using System.Text;
using System.Xml.Xsl.Qil;
using System.Xml.Xsl.XPath;

namespace System.Xml.Xsl.Xslt;

internal class QilStrConcatenator
{
	private XPathQilFactory f;

	private StringBuilder builder;

	private QilList concat;

	private bool inUse;

	public QilStrConcatenator(XPathQilFactory f)
	{
		this.f = f;
		builder = new StringBuilder();
	}

	public void Reset()
	{
		inUse = true;
		builder.Length = 0;
		concat = null;
	}

	private void FlushBuilder()
	{
		if (concat == null)
		{
			concat = f.BaseFactory.Sequence();
		}
		if (builder.Length != 0)
		{
			concat.Add(f.String(builder.ToString()));
			builder.Length = 0;
		}
	}

	public void Append(string value)
	{
		builder.Append(value);
	}

	public void Append(char value)
	{
		builder.Append(value);
	}

	public void Append(QilNode value)
	{
		if (value != null)
		{
			if (value.NodeType == QilNodeType.LiteralString)
			{
				builder.Append((string)(QilLiteral)value);
				return;
			}
			FlushBuilder();
			concat.Add(value);
		}
	}

	public QilNode ToQil()
	{
		inUse = false;
		if (concat == null)
		{
			return f.String(builder.ToString());
		}
		FlushBuilder();
		return f.StrConcat(concat);
	}
}
