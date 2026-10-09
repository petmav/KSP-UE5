namespace System.Xml.Linq;

internal struct NamespaceResolver
{
	private class NamespaceDeclaration
	{
		public string prefix;

		public XNamespace ns;

		public int scope;

		public NamespaceDeclaration prev;
	}

	private int scope;

	private NamespaceDeclaration declaration;

	private NamespaceDeclaration rover;

	public void PushScope()
	{
		scope++;
	}

	public void PopScope()
	{
		NamespaceDeclaration prev = declaration;
		if (prev != null)
		{
			do
			{
				prev = prev.prev;
				if (prev.scope != scope)
				{
					break;
				}
				if (prev == declaration)
				{
					declaration = null;
				}
				else
				{
					declaration.prev = prev.prev;
				}
				rover = null;
			}
			while (prev != declaration && declaration != null);
		}
		scope--;
	}

	public void Add(string prefix, XNamespace ns)
	{
		NamespaceDeclaration namespaceDeclaration = new NamespaceDeclaration();
		namespaceDeclaration.prefix = prefix;
		namespaceDeclaration.ns = ns;
		namespaceDeclaration.scope = scope;
		if (declaration == null)
		{
			declaration = namespaceDeclaration;
		}
		else
		{
			namespaceDeclaration.prev = declaration.prev;
		}
		declaration.prev = namespaceDeclaration;
		rover = null;
	}

	public void AddFirst(string prefix, XNamespace ns)
	{
		NamespaceDeclaration namespaceDeclaration = new NamespaceDeclaration();
		namespaceDeclaration.prefix = prefix;
		namespaceDeclaration.ns = ns;
		namespaceDeclaration.scope = scope;
		if (declaration == null)
		{
			namespaceDeclaration.prev = namespaceDeclaration;
		}
		else
		{
			namespaceDeclaration.prev = declaration.prev;
			declaration.prev = namespaceDeclaration;
		}
		declaration = namespaceDeclaration;
		rover = null;
	}

	public string GetPrefixOfNamespace(XNamespace ns, bool allowDefaultNamespace)
	{
		if (rover != null && rover.ns == ns && (allowDefaultNamespace || rover.prefix.Length > 0))
		{
			return rover.prefix;
		}
		NamespaceDeclaration prev = declaration;
		if (prev != null)
		{
			do
			{
				prev = prev.prev;
				if (!(prev.ns == ns))
				{
					continue;
				}
				NamespaceDeclaration prev2 = declaration.prev;
				while (prev2 != prev && prev2.prefix != prev.prefix)
				{
					prev2 = prev2.prev;
				}
				if (prev2 == prev)
				{
					if (allowDefaultNamespace)
					{
						rover = prev;
						return prev.prefix;
					}
					if (prev.prefix.Length > 0)
					{
						return prev.prefix;
					}
				}
			}
			while (prev != declaration);
		}
		return null;
	}
}
