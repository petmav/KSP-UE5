using System.IO;

namespace System.Xml.Xsl.Runtime;

internal class XmlQueryDataWriter : BinaryWriter
{
	public XmlQueryDataWriter(Stream output)
		: base(output)
	{
	}

	public void WriteInt32Encoded(int value)
	{
		Write7BitEncodedInt(value);
	}

	public void WriteStringQ(string value)
	{
		Write(value != null);
		if (value != null)
		{
			Write(value);
		}
	}
}
