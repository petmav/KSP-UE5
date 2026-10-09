namespace System.Xml.Serialization;

internal class XmlSerializationPrimitiveReader : XmlSerializationReader
{
	private string id4_boolean;

	private string id14_unsignedInt;

	private string id15_unsignedLong;

	private string id7_float;

	private string id10_dateTime;

	private string id6_long;

	private string id9_decimal;

	private string id8_double;

	private string id17_guid;

	private string id19_TimeSpan;

	private string id2_Item;

	private string id13_unsignedShort;

	private string id18_char;

	private string id3_int;

	private string id12_byte;

	private string id16_base64Binary;

	private string id11_unsignedByte;

	private string id5_short;

	private string id1_string;

	private string id1_QName;

	internal object Read_string()
	{
		object result = null;
		base.Reader.MoveToContent();
		if (base.Reader.NodeType == XmlNodeType.Element)
		{
			if ((object)base.Reader.LocalName != id1_string || (object)base.Reader.NamespaceURI != id2_Item)
			{
				throw CreateUnknownNodeException();
			}
			result = ((!ReadNull()) ? base.Reader.ReadElementString() : null);
		}
		else
		{
			UnknownNode(null);
		}
		return result;
	}

	internal object Read_int()
	{
		object result = null;
		base.Reader.MoveToContent();
		if (base.Reader.NodeType == XmlNodeType.Element)
		{
			if ((object)base.Reader.LocalName != id3_int || (object)base.Reader.NamespaceURI != id2_Item)
			{
				throw CreateUnknownNodeException();
			}
			result = XmlConvert.ToInt32(base.Reader.ReadElementString());
		}
		else
		{
			UnknownNode(null);
		}
		return result;
	}

	internal object Read_boolean()
	{
		object result = null;
		base.Reader.MoveToContent();
		if (base.Reader.NodeType == XmlNodeType.Element)
		{
			if ((object)base.Reader.LocalName != id4_boolean || (object)base.Reader.NamespaceURI != id2_Item)
			{
				throw CreateUnknownNodeException();
			}
			result = XmlConvert.ToBoolean(base.Reader.ReadElementString());
		}
		else
		{
			UnknownNode(null);
		}
		return result;
	}

	internal object Read_short()
	{
		object result = null;
		base.Reader.MoveToContent();
		if (base.Reader.NodeType == XmlNodeType.Element)
		{
			if ((object)base.Reader.LocalName != id5_short || (object)base.Reader.NamespaceURI != id2_Item)
			{
				throw CreateUnknownNodeException();
			}
			result = XmlConvert.ToInt16(base.Reader.ReadElementString());
		}
		else
		{
			UnknownNode(null);
		}
		return result;
	}

	internal object Read_long()
	{
		object result = null;
		base.Reader.MoveToContent();
		if (base.Reader.NodeType == XmlNodeType.Element)
		{
			if ((object)base.Reader.LocalName != id6_long || (object)base.Reader.NamespaceURI != id2_Item)
			{
				throw CreateUnknownNodeException();
			}
			result = XmlConvert.ToInt64(base.Reader.ReadElementString());
		}
		else
		{
			UnknownNode(null);
		}
		return result;
	}

	internal object Read_float()
	{
		object result = null;
		base.Reader.MoveToContent();
		if (base.Reader.NodeType == XmlNodeType.Element)
		{
			if ((object)base.Reader.LocalName != id7_float || (object)base.Reader.NamespaceURI != id2_Item)
			{
				throw CreateUnknownNodeException();
			}
			result = XmlConvert.ToSingle(base.Reader.ReadElementString());
		}
		else
		{
			UnknownNode(null);
		}
		return result;
	}

	internal object Read_double()
	{
		object result = null;
		base.Reader.MoveToContent();
		if (base.Reader.NodeType == XmlNodeType.Element)
		{
			if ((object)base.Reader.LocalName != id8_double || (object)base.Reader.NamespaceURI != id2_Item)
			{
				throw CreateUnknownNodeException();
			}
			result = XmlConvert.ToDouble(base.Reader.ReadElementString());
		}
		else
		{
			UnknownNode(null);
		}
		return result;
	}

	internal object Read_decimal()
	{
		object result = null;
		base.Reader.MoveToContent();
		if (base.Reader.NodeType == XmlNodeType.Element)
		{
			if ((object)base.Reader.LocalName != id9_decimal || (object)base.Reader.NamespaceURI != id2_Item)
			{
				throw CreateUnknownNodeException();
			}
			result = XmlConvert.ToDecimal(base.Reader.ReadElementString());
		}
		else
		{
			UnknownNode(null);
		}
		return result;
	}

	internal object Read_dateTime()
	{
		object result = null;
		base.Reader.MoveToContent();
		if (base.Reader.NodeType == XmlNodeType.Element)
		{
			if ((object)base.Reader.LocalName != id10_dateTime || (object)base.Reader.NamespaceURI != id2_Item)
			{
				throw CreateUnknownNodeException();
			}
			result = XmlSerializationReader.ToDateTime(base.Reader.ReadElementString());
		}
		else
		{
			UnknownNode(null);
		}
		return result;
	}

	internal object Read_unsignedByte()
	{
		object result = null;
		base.Reader.MoveToContent();
		if (base.Reader.NodeType == XmlNodeType.Element)
		{
			if ((object)base.Reader.LocalName != id11_unsignedByte || (object)base.Reader.NamespaceURI != id2_Item)
			{
				throw CreateUnknownNodeException();
			}
			result = XmlConvert.ToByte(base.Reader.ReadElementString());
		}
		else
		{
			UnknownNode(null);
		}
		return result;
	}

	internal object Read_byte()
	{
		object result = null;
		base.Reader.MoveToContent();
		if (base.Reader.NodeType == XmlNodeType.Element)
		{
			if ((object)base.Reader.LocalName != id12_byte || (object)base.Reader.NamespaceURI != id2_Item)
			{
				throw CreateUnknownNodeException();
			}
			result = XmlConvert.ToSByte(base.Reader.ReadElementString());
		}
		else
		{
			UnknownNode(null);
		}
		return result;
	}

	internal object Read_unsignedShort()
	{
		object result = null;
		base.Reader.MoveToContent();
		if (base.Reader.NodeType == XmlNodeType.Element)
		{
			if ((object)base.Reader.LocalName != id13_unsignedShort || (object)base.Reader.NamespaceURI != id2_Item)
			{
				throw CreateUnknownNodeException();
			}
			result = XmlConvert.ToUInt16(base.Reader.ReadElementString());
		}
		else
		{
			UnknownNode(null);
		}
		return result;
	}

	internal object Read_unsignedInt()
	{
		object result = null;
		base.Reader.MoveToContent();
		if (base.Reader.NodeType == XmlNodeType.Element)
		{
			if ((object)base.Reader.LocalName != id14_unsignedInt || (object)base.Reader.NamespaceURI != id2_Item)
			{
				throw CreateUnknownNodeException();
			}
			result = XmlConvert.ToUInt32(base.Reader.ReadElementString());
		}
		else
		{
			UnknownNode(null);
		}
		return result;
	}

	internal object Read_unsignedLong()
	{
		object result = null;
		base.Reader.MoveToContent();
		if (base.Reader.NodeType == XmlNodeType.Element)
		{
			if ((object)base.Reader.LocalName != id15_unsignedLong || (object)base.Reader.NamespaceURI != id2_Item)
			{
				throw CreateUnknownNodeException();
			}
			result = XmlConvert.ToUInt64(base.Reader.ReadElementString());
		}
		else
		{
			UnknownNode(null);
		}
		return result;
	}

	internal object Read_base64Binary()
	{
		object result = null;
		base.Reader.MoveToContent();
		if (base.Reader.NodeType == XmlNodeType.Element)
		{
			if ((object)base.Reader.LocalName != id16_base64Binary || (object)base.Reader.NamespaceURI != id2_Item)
			{
				throw CreateUnknownNodeException();
			}
			result = ((!ReadNull()) ? ToByteArrayBase64(isNull: false) : null);
		}
		else
		{
			UnknownNode(null);
		}
		return result;
	}

	internal object Read_guid()
	{
		object result = null;
		base.Reader.MoveToContent();
		if (base.Reader.NodeType == XmlNodeType.Element)
		{
			if ((object)base.Reader.LocalName != id17_guid || (object)base.Reader.NamespaceURI != id2_Item)
			{
				throw CreateUnknownNodeException();
			}
			result = XmlConvert.ToGuid(base.Reader.ReadElementString());
		}
		else
		{
			UnknownNode(null);
		}
		return result;
	}

	internal object Read_TimeSpan()
	{
		object result = null;
		base.Reader.MoveToContent();
		if (base.Reader.NodeType == XmlNodeType.Element)
		{
			if ((object)base.Reader.LocalName != id19_TimeSpan || (object)base.Reader.NamespaceURI != id2_Item)
			{
				throw CreateUnknownNodeException();
			}
			if (base.Reader.IsEmptyElement)
			{
				base.Reader.Skip();
				result = default(TimeSpan);
			}
			else
			{
				result = XmlConvert.ToTimeSpan(base.Reader.ReadElementString());
			}
		}
		else
		{
			UnknownNode(null);
		}
		return result;
	}

	internal object Read_char()
	{
		object result = null;
		base.Reader.MoveToContent();
		if (base.Reader.NodeType == XmlNodeType.Element)
		{
			if ((object)base.Reader.LocalName != id18_char || (object)base.Reader.NamespaceURI != id2_Item)
			{
				throw CreateUnknownNodeException();
			}
			result = XmlSerializationReader.ToChar(base.Reader.ReadElementString());
		}
		else
		{
			UnknownNode(null);
		}
		return result;
	}

	internal object Read_QName()
	{
		object result = null;
		base.Reader.MoveToContent();
		if (base.Reader.NodeType == XmlNodeType.Element)
		{
			if ((object)base.Reader.LocalName != id1_QName || (object)base.Reader.NamespaceURI != id2_Item)
			{
				throw CreateUnknownNodeException();
			}
			result = ((!ReadNull()) ? ReadElementQualifiedName() : null);
		}
		else
		{
			UnknownNode(null);
		}
		return result;
	}

	protected override void InitCallbacks()
	{
	}

	protected override void InitIDs()
	{
		id4_boolean = base.Reader.NameTable.Add("boolean");
		id14_unsignedInt = base.Reader.NameTable.Add("unsignedInt");
		id15_unsignedLong = base.Reader.NameTable.Add("unsignedLong");
		id7_float = base.Reader.NameTable.Add("float");
		id10_dateTime = base.Reader.NameTable.Add("dateTime");
		id6_long = base.Reader.NameTable.Add("long");
		id9_decimal = base.Reader.NameTable.Add("decimal");
		id8_double = base.Reader.NameTable.Add("double");
		id17_guid = base.Reader.NameTable.Add("guid");
		if (System.LocalAppContextSwitches.EnableTimeSpanSerialization)
		{
			id19_TimeSpan = base.Reader.NameTable.Add("TimeSpan");
		}
		id2_Item = base.Reader.NameTable.Add("");
		id13_unsignedShort = base.Reader.NameTable.Add("unsignedShort");
		id18_char = base.Reader.NameTable.Add("char");
		id3_int = base.Reader.NameTable.Add("int");
		id12_byte = base.Reader.NameTable.Add("byte");
		id16_base64Binary = base.Reader.NameTable.Add("base64Binary");
		id11_unsignedByte = base.Reader.NameTable.Add("unsignedByte");
		id5_short = base.Reader.NameTable.Add("short");
		id1_string = base.Reader.NameTable.Add("string");
		id1_QName = base.Reader.NameTable.Add("QName");
	}
}
