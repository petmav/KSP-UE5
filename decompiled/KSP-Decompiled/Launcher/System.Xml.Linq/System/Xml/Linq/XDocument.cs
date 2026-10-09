using System.IO;
using System.Text;

namespace System.Xml.Linq;

public class XDocument : XContainer
{
	private XDeclaration declaration;

	public XDeclaration Declaration
	{
		get
		{
			return declaration;
		}
		set
		{
			declaration = value;
		}
	}

	public XDocumentType DocumentType => GetFirstNode<XDocumentType>();

	public override XmlNodeType NodeType => XmlNodeType.Document;

	public XElement Root => GetFirstNode<XElement>();

	public XDocument()
	{
	}

	public XDocument(params object[] content)
		: this()
	{
		AddContentSkipNotify(content);
	}

	public XDocument(XDeclaration declaration, params object[] content)
		: this(content)
	{
		this.declaration = declaration;
	}

	public XDocument(XDocument other)
		: base(other)
	{
		if (other.declaration != null)
		{
			declaration = new XDeclaration(other.declaration);
		}
	}

	public static XDocument Load(string uri)
	{
		return Load(uri, LoadOptions.None);
	}

	public static XDocument Load(string uri, LoadOptions options)
	{
		XmlReaderSettings xmlReaderSettings = XNode.GetXmlReaderSettings(options);
		using XmlReader reader = XmlReader.Create(uri, xmlReaderSettings);
		return Load(reader, options);
	}

	public static XDocument Load(Stream stream)
	{
		return Load(stream, LoadOptions.None);
	}

	public static XDocument Load(Stream stream, LoadOptions options)
	{
		XmlReaderSettings xmlReaderSettings = XNode.GetXmlReaderSettings(options);
		using XmlReader reader = XmlReader.Create(stream, xmlReaderSettings);
		return Load(reader, options);
	}

	public static XDocument Load(TextReader textReader)
	{
		return Load(textReader, LoadOptions.None);
	}

	public static XDocument Load(TextReader textReader, LoadOptions options)
	{
		XmlReaderSettings xmlReaderSettings = XNode.GetXmlReaderSettings(options);
		using XmlReader reader = XmlReader.Create(textReader, xmlReaderSettings);
		return Load(reader, options);
	}

	public static XDocument Load(XmlReader reader)
	{
		return Load(reader, LoadOptions.None);
	}

	public static XDocument Load(XmlReader reader, LoadOptions options)
	{
		if (reader == null)
		{
			throw new ArgumentNullException("reader");
		}
		if (reader.ReadState == ReadState.Initial)
		{
			reader.Read();
		}
		XDocument xDocument = new XDocument();
		if ((options & LoadOptions.SetBaseUri) != 0)
		{
			string baseURI = reader.BaseURI;
			if (baseURI != null && baseURI.Length != 0)
			{
				xDocument.SetBaseUri(baseURI);
			}
		}
		if ((options & LoadOptions.SetLineInfo) != 0 && reader is IXmlLineInfo xmlLineInfo && xmlLineInfo.HasLineInfo())
		{
			xDocument.SetLineInfo(xmlLineInfo.LineNumber, xmlLineInfo.LinePosition);
		}
		if (reader.NodeType == XmlNodeType.XmlDeclaration)
		{
			xDocument.Declaration = new XDeclaration(reader);
		}
		xDocument.ReadContentFrom(reader, options);
		if (!reader.EOF)
		{
			throw new InvalidOperationException(Res.GetString("InvalidOperation_ExpectedEndOfFile"));
		}
		if (xDocument.Root == null)
		{
			throw new InvalidOperationException(Res.GetString("InvalidOperation_MissingRoot"));
		}
		return xDocument;
	}

	public static XDocument Parse(string text)
	{
		return Parse(text, LoadOptions.None);
	}

	public static XDocument Parse(string text, LoadOptions options)
	{
		using StringReader input = new StringReader(text);
		XmlReaderSettings xmlReaderSettings = XNode.GetXmlReaderSettings(options);
		using XmlReader reader = XmlReader.Create(input, xmlReaderSettings);
		return Load(reader, options);
	}

	public void Save(string fileName)
	{
		Save(fileName, GetSaveOptionsFromAnnotations());
	}

	public void Save(string fileName, SaveOptions options)
	{
		XmlWriterSettings xmlWriterSettings = XNode.GetXmlWriterSettings(options);
		if (declaration != null && !string.IsNullOrEmpty(declaration.Encoding))
		{
			try
			{
				xmlWriterSettings.Encoding = Encoding.GetEncoding(declaration.Encoding);
			}
			catch (ArgumentException)
			{
			}
		}
		using XmlWriter writer = XmlWriter.Create(fileName, xmlWriterSettings);
		Save(writer);
	}

	public void Save(Stream stream)
	{
		Save(stream, GetSaveOptionsFromAnnotations());
	}

	public void Save(Stream stream, SaveOptions options)
	{
		XmlWriterSettings xmlWriterSettings = XNode.GetXmlWriterSettings(options);
		if (declaration != null && !string.IsNullOrEmpty(declaration.Encoding))
		{
			try
			{
				xmlWriterSettings.Encoding = Encoding.GetEncoding(declaration.Encoding);
			}
			catch (ArgumentException)
			{
			}
		}
		using XmlWriter writer = XmlWriter.Create(stream, xmlWriterSettings);
		Save(writer);
	}

	public void Save(TextWriter textWriter)
	{
		Save(textWriter, GetSaveOptionsFromAnnotations());
	}

	public void Save(TextWriter textWriter, SaveOptions options)
	{
		XmlWriterSettings xmlWriterSettings = XNode.GetXmlWriterSettings(options);
		using XmlWriter writer = XmlWriter.Create(textWriter, xmlWriterSettings);
		Save(writer);
	}

	public void Save(XmlWriter writer)
	{
		WriteTo(writer);
	}

	public override void WriteTo(XmlWriter writer)
	{
		if (writer == null)
		{
			throw new ArgumentNullException("writer");
		}
		if (declaration != null && declaration.Standalone == "yes")
		{
			writer.WriteStartDocument(standalone: true);
		}
		else if (declaration != null && declaration.Standalone == "no")
		{
			writer.WriteStartDocument(standalone: false);
		}
		else
		{
			writer.WriteStartDocument();
		}
		WriteContentTo(writer);
		writer.WriteEndDocument();
	}

	internal override void AddAttribute(XAttribute a)
	{
		throw new ArgumentException(Res.GetString("Argument_AddAttribute"));
	}

	internal override void AddAttributeSkipNotify(XAttribute a)
	{
		throw new ArgumentException(Res.GetString("Argument_AddAttribute"));
	}

	internal override XNode CloneNode()
	{
		return new XDocument(this);
	}

	internal override bool DeepEquals(XNode node)
	{
		if (node is XDocument e)
		{
			return ContentsEqual(e);
		}
		return false;
	}

	internal override int GetDeepHashCode()
	{
		return ContentsHashCode();
	}

	private T GetFirstNode<T>() where T : XNode
	{
		XNode xNode = content as XNode;
		if (xNode != null)
		{
			do
			{
				xNode = xNode.next;
				if (xNode is T result)
				{
					return result;
				}
			}
			while (xNode != content);
		}
		return null;
	}

	internal static bool IsWhitespace(string s)
	{
		foreach (char c in s)
		{
			if (c != ' ' && c != '\t' && c != '\r' && c != '\n')
			{
				return false;
			}
		}
		return true;
	}

	internal override void ValidateNode(XNode node, XNode previous)
	{
		switch (node.NodeType)
		{
		case XmlNodeType.Text:
			ValidateString(((XText)node).Value);
			break;
		case XmlNodeType.Element:
			ValidateDocument(previous, XmlNodeType.DocumentType, XmlNodeType.None);
			break;
		case XmlNodeType.DocumentType:
			ValidateDocument(previous, XmlNodeType.None, XmlNodeType.Element);
			break;
		case XmlNodeType.CDATA:
			throw new ArgumentException(Res.GetString("Argument_AddNode", XmlNodeType.CDATA));
		case XmlNodeType.Document:
			throw new ArgumentException(Res.GetString("Argument_AddNode", XmlNodeType.Document));
		}
	}

	private void ValidateDocument(XNode previous, XmlNodeType allowBefore, XmlNodeType allowAfter)
	{
		XNode xNode = content as XNode;
		if (xNode == null)
		{
			return;
		}
		if (previous == null)
		{
			allowBefore = allowAfter;
		}
		do
		{
			xNode = xNode.next;
			XmlNodeType nodeType = xNode.NodeType;
			if (nodeType == XmlNodeType.Element || nodeType == XmlNodeType.DocumentType)
			{
				if (nodeType != allowBefore)
				{
					throw new InvalidOperationException(Res.GetString("InvalidOperation_DocumentStructure"));
				}
				allowBefore = XmlNodeType.None;
			}
			if (xNode == previous)
			{
				allowBefore = allowAfter;
			}
		}
		while (xNode != content);
	}

	internal override void ValidateString(string s)
	{
		if (!IsWhitespace(s))
		{
			throw new ArgumentException(Res.GetString("Argument_AddNonWhitespace"));
		}
	}
}
