namespace System.Xml.Serialization;

internal class NameKey
{
	private string ns;

	private string name;

	internal NameKey(string name, string ns)
	{
		this.name = name;
		this.ns = ns;
	}

	public override bool Equals(object other)
	{
		if (!(other is NameKey))
		{
			return false;
		}
		NameKey nameKey = (NameKey)other;
		if (name == nameKey.name)
		{
			return ns == nameKey.ns;
		}
		return false;
	}

	public override int GetHashCode()
	{
		return ((ns == null) ? "<null>".GetHashCode() : ns.GetHashCode()) ^ ((name != null) ? name.GetHashCode() : 0);
	}
}
