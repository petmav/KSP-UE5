using System.Configuration;
using System.Runtime.CompilerServices;
using Unity;

namespace System.Net.Configuration;

[ConfigurationCollection(typeof(AuthenticationModuleElement))]
public sealed class AuthenticationModuleElementCollection : ConfigurationElementCollection
{
	public new AuthenticationModuleElement this[string name]
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

	public AuthenticationModuleElementCollection()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	[SpecialName]
	public AuthenticationModuleElement get_Item(int index)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	[SpecialName]
	public void set_Item(int index, AuthenticationModuleElement value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Add(AuthenticationModuleElement element)
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

	public int IndexOf(AuthenticationModuleElement element)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public void Remove(AuthenticationModuleElement element)
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
