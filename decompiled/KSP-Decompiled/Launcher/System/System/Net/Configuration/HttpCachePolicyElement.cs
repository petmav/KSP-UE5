using System.Configuration;
using System.Net.Cache;
using System.Xml;
using Unity;

namespace System.Net.Configuration;

public sealed class HttpCachePolicyElement : ConfigurationElement
{
	public TimeSpan MaximumAge
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

	public TimeSpan MaximumStale
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

	public TimeSpan MinimumFresh
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

	public HttpRequestCacheLevel PolicyLevel
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(HttpRequestCacheLevel);
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

	public HttpCachePolicyElement()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected override void DeserializeElement(XmlReader reader, bool serializeCollectionKey)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected override void Reset(ConfigurationElement parentElement)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
