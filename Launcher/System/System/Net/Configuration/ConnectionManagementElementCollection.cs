using System.Configuration;
using System.Runtime.CompilerServices;
using Unity;

namespace System.Net.Configuration;

[ConfigurationCollection(typeof(ConnectionManagementElement))]
public sealed class ConnectionManagementElementCollection : ConfigurationElementCollection
{
	public new ConnectionManagementElement this[string name]
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

	public ConnectionManagementElementCollection()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	[SpecialName]
	public ConnectionManagementElement get_Item(int index)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	[SpecialName]
	public void set_Item(int index, ConnectionManagementElement value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Add(ConnectionManagementElement element)
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

	public int IndexOf(ConnectionManagementElement element)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public void Remove(ConnectionManagementElement element)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Remove(string name)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void RemoveAt(int index)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
