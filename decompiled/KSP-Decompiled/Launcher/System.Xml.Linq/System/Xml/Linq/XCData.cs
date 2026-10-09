namespace System.Xml.Linq;

public class XCData : XText
{
	public override XmlNodeType NodeType => XmlNodeType.CDATA;

	public XCData(string value)
		: base(value)
	{
	}

	public XCData(XCData other)
		: base(other)
	{
	}

	internal XCData(XmlReader r)
		: base(r)
	{
	}

	public override void WriteTo(XmlWriter writer)
	{
		if (writer == null)
		{
			throw new ArgumentNullException("writer");
		}
		writer.WriteCData(text);
	}

	internal override XNode CloneNode()
	{
		return new XCData(this);
	}
}
