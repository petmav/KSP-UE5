using System.Configuration;
using Unity;

namespace System.Runtime.Serialization.Configuration;

public sealed class NetDataContractSerializerSection : ConfigurationSection
{
	public bool EnableUnsafeTypeForwarding
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
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

	public NetDataContractSerializerSection()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
