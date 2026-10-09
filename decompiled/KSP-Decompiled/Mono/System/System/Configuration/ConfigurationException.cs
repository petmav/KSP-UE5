using System.Runtime.Serialization;
using System.Security.Permissions;
using System.Xml;

namespace System.Configuration;

[Serializable]
public class ConfigurationException : SystemException
{
	private readonly string filename;

	private readonly int line;

	public virtual string BareMessage => base.Message;

	public virtual string Filename => filename;

	public virtual int Line => line;

	public override string Message
	{
		get
		{
			if (filename != null && filename.Length != 0)
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

	[Obsolete("This class is obsolete.  Use System.Configuration.ConfigurationErrorsException")]
	public ConfigurationException()
		: this(null)
	{
		filename = null;
		line = 0;
	}

	[Obsolete("This class is obsolete.  Use System.Configuration.ConfigurationErrorsException")]
	public ConfigurationException(string message)
		: base(message)
	{
	}

	protected ConfigurationException(SerializationInfo info, StreamingContext context)
		: base(info, context)
	{
		filename = info.GetString("filename");
		line = info.GetInt32("line");
	}

	[Obsolete("This class is obsolete.  Use System.Configuration.ConfigurationErrorsException")]
	public ConfigurationException(string message, Exception inner)
		: base(message, inner)
	{
	}

	[Obsolete("This class is obsolete.  Use System.Configuration.ConfigurationErrorsException")]
	public ConfigurationException(string message, XmlNode node)
		: base(message)
	{
		filename = GetXmlNodeFilename(node);
		line = GetXmlNodeLineNumber(node);
	}

	[Obsolete("This class is obsolete.  Use System.Configuration.ConfigurationErrorsException")]
	public ConfigurationException(string message, Exception inner, XmlNode node)
		: base(message, inner)
	{
		filename = GetXmlNodeFilename(node);
		line = GetXmlNodeLineNumber(node);
	}

	[Obsolete("This class is obsolete.  Use System.Configuration.ConfigurationErrorsException")]
	public ConfigurationException(string message, string filename, int line)
		: base(message)
	{
		this.filename = filename;
		this.line = line;
	}

	[Obsolete("This class is obsolete.  Use System.Configuration.ConfigurationErrorsException")]
	public ConfigurationException(string message, Exception inner, string filename, int line)
		: base(message, inner)
	{
		this.filename = filename;
		this.line = line;
	}

	[Obsolete("This class is obsolete.  Use System.Configuration.ConfigurationErrorsException")]
	public static string GetXmlNodeFilename(XmlNode node)
	{
		if (!(node is System.Configuration.IConfigXmlNode))
		{
			return string.Empty;
		}
		return ((System.Configuration.IConfigXmlNode)node).Filename;
	}

	[Obsolete("This class is obsolete.  Use System.Configuration.ConfigurationErrorsException")]
	public static int GetXmlNodeLineNumber(XmlNode node)
	{
		if (!(node is System.Configuration.IConfigXmlNode))
		{
			return 0;
		}
		return ((System.Configuration.IConfigXmlNode)node).LineNumber;
	}

	[SecurityPermission(SecurityAction.Demand, SerializationFormatter = true)]
	public override void GetObjectData(SerializationInfo info, StreamingContext context)
	{
		base.GetObjectData(info, context);
		info.AddValue("filename", filename);
		info.AddValue("line", line);
	}
}
