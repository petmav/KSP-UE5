using System.Configuration;
using Unity;

namespace System.Runtime.Serialization.Configuration;

public sealed class DataContractSerializerSection : ConfigurationSection
{
	public DeclaredTypeElementCollection DeclaredTypes
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
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

	public DataContractSerializerSection()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
