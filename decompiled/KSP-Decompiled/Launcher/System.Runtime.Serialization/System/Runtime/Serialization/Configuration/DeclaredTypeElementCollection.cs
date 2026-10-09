using System.Configuration;
using System.Runtime.CompilerServices;
using Unity;

namespace System.Runtime.Serialization.Configuration;

[ConfigurationCollection(typeof(DeclaredTypeElement))]
public sealed class DeclaredTypeElementCollection : ConfigurationElementCollection
{
	public new DeclaredTypeElement this[string typeName]
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

	public DeclaredTypeElementCollection()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	[SpecialName]
	public DeclaredTypeElement get_Item(int index)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	[SpecialName]
	public void set_Item(int index, DeclaredTypeElement value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Add(DeclaredTypeElement element)
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

	public int IndexOf(DeclaredTypeElement element)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public void Remove(DeclaredTypeElement element)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Remove(string typeName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void RemoveAt(int index)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
