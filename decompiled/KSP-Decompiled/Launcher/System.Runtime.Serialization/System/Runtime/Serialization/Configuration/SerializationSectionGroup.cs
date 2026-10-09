using System.Configuration;
using Unity;

namespace System.Runtime.Serialization.Configuration;

public sealed class SerializationSectionGroup : ConfigurationSectionGroup
{
	public DataContractSerializerSection DataContractSerializer
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public NetDataContractSerializerSection NetDataContractSerializer
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public SerializationSectionGroup()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public static SerializationSectionGroup GetSectionGroup(System.Configuration.Configuration config)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}
}
