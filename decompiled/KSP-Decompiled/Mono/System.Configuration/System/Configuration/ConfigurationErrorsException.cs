using System.Collections;
using System.Configuration.Internal;
using System.Runtime.Serialization;
using System.Security.Permissions;
using System.Xml;

namespace System.Configuration;

[Serializable]
public class ConfigurationErrorsException : ConfigurationException
{
	private readonly string filename;

	private readonly int line;

	public override string BareMessage => base.BareMessage;

	public ICollection Errors
	{
		get
		{
			throw new NotImplementedException();
		}
	}

	public override string Filename => filename;

	public override int Line => line;

	public override string Message
	{
		get
		{
			if (!string.IsNullOrEmpty(filename))
			{
				if (line != 0)
				{
					return BareMessage + " (" + filename + " line " + line + ")";
				}
				return BareMessage + " (" + filename + ")";
			}
			if (line != 0)
			{
				return BareMessage + " (line " + line + ")";
			}
			return BareMessage;
		}
	}

	public ConfigurationErrorsException()
	{
	}

	public ConfigurationErrorsException(string message)
		: base(message)
	{
	}

	protected ConfigurationErrorsException(SerializationInfo info, StreamingContext context)
		: base(info, context)
	{
		filename = info.GetString("ConfigurationErrors_Filename");
		line = info.GetInt32("ConfigurationErrors_Line");
	}

	public ConfigurationErrorsException(string message, Exception inner)
		: base(message, inner)
	{
	}

	public ConfigurationErrorsException(string message, XmlNode node)
		: this(message, null, GetFilename(node), GetLineNumber(node))
	{
	}

	public ConfigurationErrorsException(string message, Exception inner, XmlNode node)
		: this(message, inner, GetFilename(node), GetLineNumber(node))
	{
	}

	public ConfigurationErrorsException(string message, XmlReader reader)
		: this(message, null, GetFilename(reader), GetLineNumber(reader))
	{
	}

	public ConfigurationErrorsException(string message, Exception inner, XmlReader reader)
		: this(message, inner, GetFilename(reader), GetLineNumber(reader))
	{
	}

	public ConfigurationErrorsException(string message, string filename, int line)
		: this(message, null, filename, line)
	{
	}

	public ConfigurationErrorsException(string message, Exception inner, string filename, int line)
		: base(message, inner)
	{
		this.filename = filename;
		this.line = line;
	}

	public static string GetFilename(XmlReader reader)
	{
		if (reader is IConfigErrorInfo)
		{
			return ((IConfigErrorInfo)reader).Filename;
		}
		return reader?.BaseURI;
	}

	public static int GetLineNumber(XmlReader reader)
	{
		if (reader is IConfigErrorInfo)
		{
			return ((IConfigErrorInfo)reader).LineNumber;
		}
		if (!(reader is IXmlLineInfo xmlLineInfo))
		{
			return 0;
		}
		return xmlLineInfo.LineNumber;
	}

	public static string GetFilename(XmlNode node)
	{
		if (!(node is IConfigErrorInfo))
		{
			return null;
		}
		return ((IConfigErrorInfo)node).Filename;
	}

	public static int GetLineNumber(XmlNode node)
	{
		if (!(node is IConfigErrorInfo))
		{
			return 0;
		}
		return ((IConfigErrorInfo)node).LineNumber;
	}

	[SecurityPermission(SecurityAction.Demand, SerializationFormatter = true)]
	public override void GetObjectData(SerializationInfo info, StreamingContext context)
	{
		base.GetObjectData(info, context);
		info.AddValue("ConfigurationErrors_Filename", filename);
		info.AddValue("ConfigurationErrors_Line", line);
	}
}
