using System.Configuration;
using Unity;

namespace System.Runtime.Serialization.Configuration;

[ConfigurationCollection(typeof(TypeElement), CollectionType = ConfigurationElementCollectionType.BasicMap)]
public sealed class TypeElementCollection : ConfigurationElementCollection
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

	public TypeElement this[int index]
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

	public TypeElementCollection()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Add(TypeElement element)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Clear()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
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

	public int IndexOf(TypeElement element)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public void Remove(TypeElement element)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void RemoveAt(int index)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
