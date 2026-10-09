using System.Configuration;
using System.Security;
using Unity;

namespace System.Runtime.Serialization.Configuration;

public sealed class DeclaredTypeElement : ConfigurationElement
{
	public TypeElementCollection KnownTypes
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

	public string Type
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public DeclaredTypeElement()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public DeclaredTypeElement(string typeName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	[SecuritySafeCritical]
	protected override void PostDeserialize()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
