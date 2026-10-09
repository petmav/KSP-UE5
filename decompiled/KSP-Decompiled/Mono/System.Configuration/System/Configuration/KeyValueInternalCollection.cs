using System.Collections.Specialized;

namespace System.Configuration;

internal class KeyValueInternalCollection : NameValueCollection
{
	public void SetReadOnly()
	{
		base.IsReadOnly = true;
	}

	public override void Add(string name, string val)
	{
		Remove(name);
		base.Add(name, val);
	}
}
