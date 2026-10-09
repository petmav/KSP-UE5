namespace System.Xml.Linq;

internal struct NamespaceCache
{
	private XNamespace ns;

	private string namespaceName;

	public XNamespace Get(string namespaceName)
	{
		if ((object)namespaceName == this.namespaceName)
		{
			return ns;
		}
		this.namespaceName = namespaceName;
		ns = XNamespace.Get(namespaceName);
		return ns;
	}
}
