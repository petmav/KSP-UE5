using System.Configuration;
using System.Xml;
using Unity;

namespace System.Runtime.Serialization.Configuration;

public sealed class ParameterElement : ConfigurationElement
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

	public ParameterElement()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public ParameterElement(int index)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public ParameterElement(string typeName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected override void PostDeserialize()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected override void PreSerialize(XmlWriter writer)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
