using System.Configuration;
using Unity;

namespace System.Runtime.Serialization.Configuration;

public sealed class TypeElement : ConfigurationElement
{
	public int Index
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

	public ParameterElementCollection Parameters
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

	public TypeElement()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public TypeElement(string typeName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected override void Reset(ConfigurationElement parentElement)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
