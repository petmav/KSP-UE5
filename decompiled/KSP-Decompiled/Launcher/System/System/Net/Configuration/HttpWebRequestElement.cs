using System.Configuration;
using Unity;

namespace System.Net.Configuration;

public sealed class HttpWebRequestElement : ConfigurationElement
{
	public int MaximumErrorResponseLength
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public int MaximumResponseHeadersLength
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public int MaximumUnauthorizedUploadLength
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	protected override ConfigurationPropertyCollection Properties
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public bool UseUnsafeHeaderParsing
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public HttpWebRequestElement()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected override void PostDeserialize()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
