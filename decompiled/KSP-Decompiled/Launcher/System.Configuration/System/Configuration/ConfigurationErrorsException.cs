using System.Collections;
using System.Runtime.Serialization;
using System.Xml;
using Unity;

namespace System.Configuration;

[Serializable]
public class ConfigurationErrorsException : ConfigurationException
{
	public override string BareMessage
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public ICollection Errors
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public override string Filename
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public override int Line
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
	}

	public ConfigurationErrorsException()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected ConfigurationErrorsException(SerializationInfo info, StreamingContext context)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public ConfigurationErrorsException(string message)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public ConfigurationErrorsException(string message, Exception inner)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public ConfigurationErrorsException(string message, Exception inner, string filename, int line)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public ConfigurationErrorsException(string message, Exception inner, XmlNode node)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public ConfigurationErrorsException(string message, Exception inner, XmlReader reader)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public ConfigurationErrorsException(string message, string filename, int line)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public ConfigurationErrorsException(string message, XmlNode node)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public ConfigurationErrorsException(string message, XmlReader reader)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public static string GetFilename(XmlNode node)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public static string GetFilename(XmlReader reader)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public static int GetLineNumber(XmlNode node)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public static int GetLineNumber(XmlReader reader)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}
}
