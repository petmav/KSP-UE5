using System.Collections;
using System.Collections.Specialized;

namespace System.Configuration;

internal class ConfigNameValueCollection : NameValueCollection
{
	private bool modified;

	public bool IsModified => modified;

	public ConfigNameValueCollection()
	{
	}

	public ConfigNameValueCollection(System.Configuration.ConfigNameValueCollection col)
		: base(col.Count, col)
	{
	}

	public ConfigNameValueCollection(IHashCodeProvider hashProvider, IComparer comparer)
		: base(hashProvider, comparer)
	{
	}

	public void ResetModified()
	{
		modified = false;
	}

	public override void Set(string name, string value)
	{
		base.Set(name, value);
		modified = true;
	}
}
