using System.Runtime.Serialization;
using System.Xml;
using Unity;

namespace System.Configuration;

[Serializable]
public class ConfigurationException : SystemException
{
	public virtual string BareMessage
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public virtual string Filename
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public virtual int Line
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
	}

	[Obsolete("This class is obsolete, to create a new exception create a System.Configuration!System.Configuration.ConfigurationErrorsException")]
	public ConfigurationException()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected ConfigurationException(SerializationInfo info, StreamingContext context)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	[Obsolete("This class is obsolete, to create a new exception create a System.Configuration!System.Configuration.ConfigurationErrorsException")]
	public ConfigurationException(string message)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	[Obsolete("This class is obsolete, to create a new exception create a System.Configuration!System.Configuration.ConfigurationErrorsException")]
	public ConfigurationException(string message, Exception inner)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	[Obsolete("This class is obsolete, to create a new exception create a System.Configuration!System.Configuration.ConfigurationErrorsException")]
	public ConfigurationException(string message, Exception inner, string filename, int line)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	[Obsolete("This class is obsolete, to create a new exception create a System.Configuration!System.Configuration.ConfigurationErrorsException")]
	public ConfigurationException(string message, Exception inner, XmlNode node)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	[Obsolete("This class is obsolete, to create a new exception create a System.Configuration!System.Configuration.ConfigurationErrorsException")]
	public ConfigurationException(string message, string filename, int line)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	[Obsolete("This class is obsolete, to create a new exception create a System.Configuration!System.Configuration.ConfigurationErrorsException")]
	public ConfigurationException(string message, XmlNode node)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	[Obsolete("This class is obsolete, use System.Configuration!System.Configuration.ConfigurationErrorsException.GetFilename instead")]
	public static string GetXmlNodeFilename(XmlNode node)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	[Obsolete("This class is obsolete, use System.Configuration!System.Configuration.ConfigurationErrorsException.GetLinenumber instead")]
	public static int GetXmlNodeLineNumber(XmlNode node)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}
}
