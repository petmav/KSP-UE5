namespace System.Xml.Serialization;

internal class TempAssemblyCacheKey
{
	private string ns;

	private object type;

	internal TempAssemblyCacheKey(string ns, object type)
	{
		this.type = type;
		this.ns = ns;
	}

	public override bool Equals(object o)
	{
		if (!(o is TempAssemblyCacheKey tempAssemblyCacheKey))
		{
			return false;
		}
		if (tempAssemblyCacheKey.type == type)
		{
			return tempAssemblyCacheKey.ns == ns;
		}
		return false;
	}

	public override int GetHashCode()
	{
		return ((ns != null) ? ns.GetHashCode() : 0) ^ ((type != null) ? type.GetHashCode() : 0);
	}
}
