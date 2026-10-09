using System.Configuration;
using Unity;

namespace System.Runtime.Serialization.Configuration;

[ConfigurationCollection(typeof(ParameterElement), AddItemName = "parameter", CollectionType = ConfigurationElementCollectionType.BasicMap)]
public sealed class ParameterElementCollection : ConfigurationElementCollection
{
	public override ConfigurationElementCollectionType CollectionType
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(ConfigurationElementCollectionType);
		}
	}

	protected override string ElementName
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public ParameterElement this[int index]
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

	public ParameterElementCollection()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Add(ParameterElement element)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Clear()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public bool Contains(string typeName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	protected override ConfigurationElement CreateNewElement()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	protected override object GetElementKey(ConfigurationElement element)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public int IndexOf(ParameterElement element)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public void Remove(ParameterElement element)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void RemoveAt(int index)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
