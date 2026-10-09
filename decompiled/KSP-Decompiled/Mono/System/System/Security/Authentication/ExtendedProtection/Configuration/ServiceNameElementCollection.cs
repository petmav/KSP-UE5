using System.Configuration;
using System.Runtime.CompilerServices;
using Unity;

namespace System.Security.Authentication.ExtendedProtection.Configuration;

[ConfigurationCollection(typeof(ServiceNameElement))]
public sealed class ServiceNameElementCollection : ConfigurationElementCollection
{
	public ServiceNameElement this[int index] => (ServiceNameElement)BaseGet(index);

	public new ServiceNameElement this[string name] => (ServiceNameElement)BaseGet(name);

	public new string this[string name]
	{
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public void Add(ServiceNameElement element)
	{
		throw new NotImplementedException();
	}

	public void Clear()
	{
		throw new NotImplementedException();
	}

	protected override ConfigurationElement CreateNewElement()
	{
		return new ServiceNameElement();
	}

	protected override object GetElementKey(ConfigurationElement element)
	{
		if (element == null)
		{
			throw new ArgumentNullException("element");
		}
		return ((ServiceNameElement)element).Name;
	}

	public int IndexOf(ServiceNameElement element)
	{
		throw new NotImplementedException();
	}

	public void Remove(string name)
	{
		throw new NotImplementedException();
	}

	public void Remove(ServiceNameElement element)
	{
		throw new NotImplementedException();
	}

	public void RemoveAt(int index)
	{
		throw new NotImplementedException();
	}

	[SpecialName]
	public void set_Item(int index, ServiceNameElement value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
