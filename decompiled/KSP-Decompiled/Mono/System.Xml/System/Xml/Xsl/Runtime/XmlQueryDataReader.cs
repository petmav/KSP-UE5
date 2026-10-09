using System.IO;

namespace System.Xml.Xsl.Runtime;

internal class XmlQueryDataReader : BinaryReader
{
	public XmlQueryDataReader(Stream input)
		: base(input)
	{
	}

	public int ReadInt32Encoded()
	{
		return Read7BitEncodedInt();
	}

	public string ReadStringQ()
	{
		if (!ReadBoolean())
		{
			return null;
		}
		return ReadString();
	}

	public sbyte ReadSByte(sbyte minValue, sbyte maxValue)
	{
		sbyte b = ReadSByte();
		if (b < minValue)
		{
			throw new ArgumentOutOfRangeException("minValue");
		}
		if (maxValue < b)
		{
			throw new ArgumentOutOfRangeException("maxValue");
		}
		return b;
	}
}
