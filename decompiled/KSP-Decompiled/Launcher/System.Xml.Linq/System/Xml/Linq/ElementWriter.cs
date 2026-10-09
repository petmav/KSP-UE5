namespace System.Xml.Linq;

internal struct ElementWriter
{
	private XmlWriter writer;

	private NamespaceResolver resolver;

	public ElementWriter(XmlWriter writer)
	{
		this.writer = writer;
		resolver = default(NamespaceResolver);
	}

	public void WriteElement(XElement e)
	{
		PushAncestors(e);
		XElement xElement = e;
		XNode xNode = e;
		while (true)
		{
			e = xNode as XElement;
			if (e != null)
			{
				WriteStartElement(e);
				if (e.content == null)
				{
					WriteEndElement();
				}
				else
				{
					if (!(e.content is string text))
					{
						xNode = ((XNode)e.content).next;
						continue;
					}
					writer.WriteString(text);
					WriteFullEndElement();
				}
			}
			else
			{
				xNode.WriteTo(writer);
			}
			while (xNode != xElement && xNode == xNode.parent.content)
			{
				xNode = xNode.parent;
				WriteFullEndElement();
			}
			if (xNode != xElement)
			{
				xNode = xNode.next;
				continue;
			}
			break;
		}
	}

	private string GetPrefixOfNamespace(XNamespace ns, bool allowDefaultNamespace)
	{
		string namespaceName = ns.NamespaceName;
		if (namespaceName.Length == 0)
		{
			return string.Empty;
		}
		string prefixOfNamespace = resolver.GetPrefixOfNamespace(ns, allowDefaultNamespace);
		if (prefixOfNamespace != null)
		{
			return prefixOfNamespace;
		}
		if ((object)namespaceName == "http://www.w3.org/XML/1998/namespace")
		{
			return "xml";
		}
		if ((object)namespaceName == "http://www.w3.org/2000/xmlns/")
		{
			return "xmlns";
		}
		return null;
	}

	private void PushAncestors(XElement e)
	{
		while (true)
		{
			e = e.parent as XElement;
			if (e == null)
			{
				break;
			}
			XAttribute xAttribute = e.lastAttr;
			if (xAttribute == null)
			{
				continue;
			}
			do
			{
				xAttribute = xAttribute.next;
				if (xAttribute.IsNamespaceDeclaration)
				{
					resolver.AddFirst((xAttribute.Name.NamespaceName.Length == 0) ? string.Empty : xAttribute.Name.LocalName, XNamespace.Get(xAttribute.Value));
				}
			}
			while (xAttribute != e.lastAttr);
		}
	}

	private void PushElement(XElement e)
	{
		resolver.PushScope();
		XAttribute xAttribute = e.lastAttr;
		if (xAttribute == null)
		{
			return;
		}
		do
		{
			xAttribute = xAttribute.next;
			if (xAttribute.IsNamespaceDeclaration)
			{
				resolver.Add((xAttribute.Name.NamespaceName.Length == 0) ? string.Empty : xAttribute.Name.LocalName, XNamespace.Get(xAttribute.Value));
			}
		}
		while (xAttribute != e.lastAttr);
	}

	private void WriteEndElement()
	{
		writer.WriteEndElement();
		resolver.PopScope();
	}

	private void WriteFullEndElement()
	{
		writer.WriteFullEndElement();
		resolver.PopScope();
	}

	private void WriteStartElement(XElement e)
	{
		PushElement(e);
		XNamespace @namespace = e.Name.Namespace;
		writer.WriteStartElement(GetPrefixOfNamespace(@namespace, allowDefaultNamespace: true), e.Name.LocalName, @namespace.NamespaceName);
		XAttribute xAttribute = e.lastAttr;
		if (xAttribute != null)
		{
			do
			{
				xAttribute = xAttribute.next;
				@namespace = xAttribute.Name.Namespace;
				string localName = xAttribute.Name.LocalName;
				string namespaceName = @namespace.NamespaceName;
				writer.WriteAttributeString(GetPrefixOfNamespace(@namespace, allowDefaultNamespace: false), localName, (namespaceName.Length == 0 && localName == "xmlns") ? "http://www.w3.org/2000/xmlns/" : namespaceName, xAttribute.Value);
			}
			while (xAttribute != e.lastAttr);
		}
	}
}
