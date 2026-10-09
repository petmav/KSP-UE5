using System.Collections;
using System.Collections.Generic;
using System.Configuration;

namespace System.CodeDom.Compiler;

[ConfigurationCollection(typeof(CompilerProviderOption), CollectionType = ConfigurationElementCollectionType.BasicMap, AddItemName = "providerOption")]
internal sealed class CompilerProviderOptionsCollection : ConfigurationElementCollection
{
	private static ConfigurationPropertyCollection properties;

	public string[] AllKeys
	{
		get
		{
			int count = base.Count;
			string[] array = new string[count];
			for (int i = 0; i < count; i++)
			{
				array[i] = this[i].Name;
			}
			return array;
		}
	}

	protected override string ElementName => "providerOption";

	protected override ConfigurationPropertyCollection Properties => properties;

	public Dictionary<string, string> ProviderOptions
	{
		get
		{
			int count = base.Count;
			if (count == 0)
			{
				return null;
			}
			Dictionary<string, string> dictionary = new Dictionary<string, string>(count);
			for (int i = 0; i < count; i++)
			{
				CompilerProviderOption compilerProviderOption = this[i];
				dictionary.Add(compilerProviderOption.Name, compilerProviderOption.Value);
			}
			return dictionary;
		}
	}

	public CompilerProviderOption this[int index] => (CompilerProviderOption)BaseGet(index);

	public new CompilerProviderOption this[string name]
	{
		get
		{
			IEnumerator enumerator = GetEnumerator();
			try
			{
				while (enumerator.MoveNext())
				{
					CompilerProviderOption compilerProviderOption = (CompilerProviderOption)enumerator.Current;
					if (compilerProviderOption.Name == name)
					{
						return compilerProviderOption;
					}
				}
			}
			finally
			{
				IDisposable disposable = enumerator as IDisposable;
				if (disposable != null)
				{
					disposable.Dispose();
				}
			}
			return null;
		}
	}

	static CompilerProviderOptionsCollection()
	{
		properties = new ConfigurationPropertyCollection();
	}

	protected override ConfigurationElement CreateNewElement()
	{
		return new CompilerProviderOption();
	}

	public CompilerProviderOption Get(int index)
	{
		return (CompilerProviderOption)BaseGet(index);
	}

	public CompilerProviderOption Get(string name)
	{
		return (CompilerProviderOption)BaseGet(name);
	}

	protected override object GetElementKey(ConfigurationElement element)
	{
		return ((CompilerProviderOption)element).Name;
	}

	public string GetKey(int index)
	{
		return (string)BaseGetKey(index);
	}
}
