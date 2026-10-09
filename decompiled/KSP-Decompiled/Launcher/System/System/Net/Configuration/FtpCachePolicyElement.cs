using System.Configuration;
using System.Net.Cache;
using System.Xml;
using Unity;

namespace System.Net.Configuration;

public sealed class FtpCachePolicyElement : ConfigurationElement
{
	public RequestCacheLevel PolicyLevel
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

	protected override ConfigurationPropertyCollection Properties
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public FtpCachePolicyElement()
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
