using System.Collections.Generic;

namespace System.Xml.Linq;

internal class XNodeBuilder : XmlWriter
{
	private List<object> content;

	private XContainer parent;

	private XName attrName;

	private string attrValue;

	private XContainer root;

	public override XmlWriterSettings Settings => new XmlWriterSettings
	{
		ConformanceLevel = ConformanceLevel.Auto
	};

	public override WriteState WriteState
	{
		get
		{
			throw new NotSupportedException();
		}
	}

	public XNodeBuilder(XContainer container)
	{
		root = container;
	}

	public override void Close()
	{
		root.Add(content);
	}

	public override void Flush()
	{
	}

	public override string LookupPrefix(string namespaceName)
	{
		throw new NotSupportedException();
	}

	public override void WriteBase64(byte[] buffer, int index, int count)
	{
		throw new NotSupportedException(Res.GetString("NotSupported_WriteBase64"));
	}

	public override void WriteCData(string text)
	{
		AddNode(new XCData(text));
	}

	public override void WriteCharEntity(char ch)
	{
		AddString(new string(ch, 1));
	}

	public override void WriteChars(char[] buffer, int index, int count)
	{
		AddString(new string(buffer, index, count));
	}

	public override void WriteComment(string text)
	{
		AddNode(new XComment(text));
	}

	public override void WriteDocType(string name, string pubid, string sysid, string subset)
	{
		AddNode(new XDocumentType(name, pubid, sysid, subset));
	}

	public override void WriteEndAttribute()
	{
		XAttribute o = new XAttribute(attrName, attrValue);
		attrName = null;
		attrValue = null;
		if (parent != null)
		{
			parent.Add(o);
		}
		else
		{
			Add(o);
		}
	}

	public override void WriteEndDocument()
	{
	}

	public override void WriteEndElement()
	{
		parent = ((XElement)parent).parent;
	}

	public override void WriteEntityRef(string name)
	{
		switch (name)
		{
		case "amp":
			AddString("&");
			break;
		case "apos":
			AddString("'");
			break;
		case "gt":
			AddString(">");
			break;
		case "lt":
			AddString("<");
			break;
		case "quot":
			AddString("\"");
			break;
		default:
			throw new NotSupportedException(Res.GetString("NotSupported_WriteEntityRef"));
		}
	}

	public override void WriteFullEndElement()
	{
		XElement xElement = (XElement)parent;
		if (xElement.IsEmpty)
		{
			xElement.Add(string.Empty);
		}
		parent = xElement.parent;
	}

	public override void WriteProcessingInstruction(string name, string text)
	{
		if (!(name == "xml"))
		{
			AddNode(new XProcessingInstruction(name, text));
		}
	}

	public override void WriteRaw(char[] buffer, int index, int count)
	{
		AddString(new string(buffer, index, count));
	}

	public override void WriteRaw(string data)
	{
		AddString(data);
	}

	public override void WriteStartAttribute(string prefix, string localName, string namespaceName)
	{
		if (prefix == null)
		{
			throw new ArgumentNullException("prefix");
		}
		attrName = XNamespace.Get((prefix.Length == 0) ? string.Empty : namespaceName).GetName(localName);
		attrValue = string.Empty;
	}

	public override void WriteStartDocument()
	{
	}

	public override void WriteStartDocument(bool standalone)
	{
	}

	public override void WriteStartElement(string prefix, string localName, string namespaceName)
	{
		AddNode(new XElement(XNamespace.Get(namespaceName).GetName(localName)));
	}

	public override void WriteString(string text)
	{
		AddString(text);
	}

	public override void WriteSurrogateCharEntity(char lowCh, char highCh)
	{
		AddString(new string(new char[2] { highCh, lowCh }));
	}

	public override void WriteValue(DateTimeOffset value)
	{
		WriteString(XmlConvert.ToString(value));
	}

	public override void WriteWhitespace(string ws)
	{
		AddString(ws);
	}

	private void Add(object o)
	{
		if (content == null)
		{
			content = new List<object>();
		}
		content.Add(o);
	}

	private void AddNode(XNode n)
	{
		if (parent != null)
		{
			parent.Add(n);
		}
		else
		{
			Add(n);
		}
		if (n is XContainer xContainer)
		{
			parent = xContainer;
		}
	}

	private void AddString(string s)
	{
		if (s != null)
		{
			if (attrValue != null)
			{
				attrValue += s;
			}
			else if (parent != null)
			{
				parent.Add(s);
			}
			else
			{
				Add(s);
			}
		}
	}
}
