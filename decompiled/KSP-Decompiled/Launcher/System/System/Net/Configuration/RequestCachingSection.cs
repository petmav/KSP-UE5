using System.Configuration;
using System.Net.Cache;
using System.Xml;
using Unity;

namespace System.Net.Configuration;

public sealed class RequestCachingSection : ConfigurationSection
{
	public FtpCachePolicyElement DefaultFtpCachePolicy
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public HttpCachePolicyElement DefaultHttpCachePolicy
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public RequestCacheLevel DefaultPolicyLevel
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(RequestCacheLevel);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public bool DisableAllCaching
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

	public bool IsPrivateCache
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

	protected override ConfigurationPropertyCollection Properties
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public TimeSpan UnspecifiedMaximumAge
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(TimeSpan);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public RequestCachingSection()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected override void DeserializeElement(XmlReader reader, bool serializeCollectionKey)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected override void PostDeserialize()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
