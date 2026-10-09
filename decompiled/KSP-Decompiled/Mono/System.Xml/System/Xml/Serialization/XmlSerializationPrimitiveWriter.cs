namespace System.Xml.Serialization;

internal class XmlSerializationPrimitiveWriter : XmlSerializationWriter
{
	internal void Write_string(object o)
	{
		WriteStartDocument();
		if (o == null)
		{
			WriteNullTagLiteral("string", "");
			return;
		}
		TopLevelElement();
		WriteNullableStringLiteral("string", "", (string)o);
	}

	internal void Write_int(object o)
	{
		WriteStartDocument();
		if (o == null)
		{
			WriteEmptyTag("int", "");
		}
		else
		{
			WriteElementStringRaw("int", "", XmlConvert.ToString((int)o));
		}
	}

	internal void Write_boolean(object o)
	{
		WriteStartDocument();
		if (o == null)
		{
			WriteEmptyTag("boolean", "");
		}
		else
		{
			WriteElementStringRaw("boolean", "", XmlConvert.ToString((bool)o));
		}
	}

	internal void Write_short(object o)
	{
		WriteStartDocument();
		if (o == null)
		{
			WriteEmptyTag("short", "");
		}
		else
		{
			WriteElementStringRaw("short", "", XmlConvert.ToString((short)o));
		}
	}

	internal void Write_long(object o)
	{
		WriteStartDocument();
		if (o == null)
		{
			WriteEmptyTag("long", "");
		}
		else
		{
			WriteElementStringRaw("long", "", XmlConvert.ToString((long)o));
		}
	}

	internal void Write_float(object o)
	{
		WriteStartDocument();
		if (o == null)
		{
			WriteEmptyTag("float", "");
		}
		else
		{
			WriteElementStringRaw("float", "", XmlConvert.ToString((float)o));
		}
	}

	internal void Write_double(object o)
	{
		WriteStartDocument();
		if (o == null)
		{
			WriteEmptyTag("double", "");
		}
		else
		{
			WriteElementStringRaw("double", "", XmlConvert.ToString((double)o));
		}
	}

	internal void Write_decimal(object o)
	{
		WriteStartDocument();
		if (o == null)
		{
			WriteEmptyTag("decimal", "");
		}
		else
		{
			WriteElementStringRaw("decimal", "", XmlConvert.ToString((decimal)o));
		}
	}

	internal void Write_dateTime(object o)
	{
		WriteStartDocument();
		if (o == null)
		{
			WriteEmptyTag("dateTime", "");
		}
		else
		{
			WriteElementStringRaw("dateTime", "", XmlSerializationWriter.FromDateTime((DateTime)o));
		}
	}

	internal void Write_unsignedByte(object o)
	{
		WriteStartDocument();
		if (o == null)
		{
			WriteEmptyTag("unsignedByte", "");
		}
		else
		{
			WriteElementStringRaw("unsignedByte", "", XmlConvert.ToString((byte)o));
		}
	}

	internal void Write_byte(object o)
	{
		WriteStartDocument();
		if (o == null)
		{
			WriteEmptyTag("byte", "");
		}
		else
		{
			WriteElementStringRaw("byte", "", XmlConvert.ToString((sbyte)o));
		}
	}

	internal void Write_unsignedShort(object o)
	{
		WriteStartDocument();
		if (o == null)
		{
			WriteEmptyTag("unsignedShort", "");
		}
		else
		{
			WriteElementStringRaw("unsignedShort", "", XmlConvert.ToString((ushort)o));
		}
	}

	internal void Write_unsignedInt(object o)
	{
		WriteStartDocument();
		if (o == null)
		{
			WriteEmptyTag("unsignedInt", "");
		}
		else
		{
			WriteElementStringRaw("unsignedInt", "", XmlConvert.ToString((uint)o));
		}
	}

	internal void Write_unsignedLong(object o)
	{
		WriteStartDocument();
		if (o == null)
		{
			WriteEmptyTag("unsignedLong", "");
		}
		else
		{
			WriteElementStringRaw("unsignedLong", "", XmlConvert.ToString((ulong)o));
		}
	}

	internal void Write_base64Binary(object o)
	{
		WriteStartDocument();
		if (o == null)
		{
			WriteNullTagLiteral("base64Binary", "");
			return;
		}
		TopLevelElement();
		WriteNullableStringLiteralRaw("base64Binary", "", XmlSerializationWriter.FromByteArrayBase64((byte[])o));
	}

	internal void Write_guid(object o)
	{
		WriteStartDocument();
		if (o == null)
		{
			WriteEmptyTag("guid", "");
		}
		else
		{
			WriteElementStringRaw("guid", "", XmlConvert.ToString((Guid)o));
		}
	}

	internal void Write_TimeSpan(object o)
	{
		WriteStartDocument();
		if (o == null)
		{
			WriteEmptyTag("TimeSpan", "");
			return;
		}
		TimeSpan timeSpan = (TimeSpan)o;
		WriteElementStringRaw("TimeSpan", "", XmlConvert.ToString(timeSpan));
	}

	internal void Write_char(object o)
	{
		WriteStartDocument();
		if (o == null)
		{
			WriteEmptyTag("char", "");
		}
		else
		{
			WriteElementString("char", "", XmlSerializationWriter.FromChar((char)o));
		}
	}

	internal void Write_QName(object o)
	{
		WriteStartDocument();
		if (o == null)
		{
			WriteNullTagLiteral("QName", "");
			return;
		}
		TopLevelElement();
		WriteNullableQualifiedNameLiteral("QName", "", (XmlQualifiedName)o);
	}

	protected override void InitCallbacks()
	{
	}
}
