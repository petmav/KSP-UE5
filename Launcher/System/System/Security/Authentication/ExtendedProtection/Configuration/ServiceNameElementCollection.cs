using System.Configuration;
using System.Runtime.CompilerServices;
using Unity;

namespace System.Security.Authentication.ExtendedProtection.Configuration;

[ConfigurationCollection(typeof(ServiceNameElement))]
public sealed class ServiceNameElementCollection : ConfigurationElementCollection
{
	public new ServiceNameElement this[string name]
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

	public ServiceNameElementCollection()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	[SpecialName]
	public ServiceNameElement get_Item(int index)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	[SpecialName]
	public void set_Item(int index, ServiceNameElement value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Add(ServiceNameElement element)
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

	public int IndexOf(ServiceNameElement element)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public void Remove(ServiceNameElement element)
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
