using System.Xml.XPath;

namespace System.Xml.Xsl.Runtime;

internal class XmlCachedSequenceWriter : XmlSequenceWriter
{
	private XmlQueryItemSequence seqTyped;

	private XPathDocument doc;

	private XmlRawWriter writer;

	public XmlQueryItemSequence ResultSequence => seqTyped;

	public XmlCachedSequenceWriter()
	{
		seqTyped = new XmlQueryItemSequence();
	}

	public override XmlRawWriter StartTree(XPathNodeType rootType, IXmlNamespaceResolver nsResolver, XmlNameTable nameTable)
	{
		doc = new XPathDocument(nameTable);
		writer = doc.LoadFromWriter(XPathDocument.LoadFlags.AtomizeNames | ((rootType != 0) ? XPathDocument.LoadFlags.Fragment : XPathDocument.LoadFlags.None), string.Empty);
		writer.NamespaceResolver = nsResolver;
		return writer;
	}

	public override void EndTree()
	{
		writer.Close();
		seqTyped.Add(doc.CreateNavigator());
	}

	public override void WriteItem(XPathItem item)
	{
		seqTyped.AddClone(item);
	}
}
