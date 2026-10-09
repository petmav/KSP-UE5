using System.Xml.XPath;

namespace System.Xml.Xsl.Runtime;

internal class XmlMergeSequenceWriter : XmlSequenceWriter
{
	private XmlRawWriter xwrt;

	private bool lastItemWasAtomic;

	public XmlMergeSequenceWriter(XmlRawWriter xwrt)
	{
		this.xwrt = xwrt;
		lastItemWasAtomic = false;
	}

	public override XmlRawWriter StartTree(XPathNodeType rootType, IXmlNamespaceResolver nsResolver, XmlNameTable nameTable)
	{
		if (rootType == XPathNodeType.Attribute || rootType == XPathNodeType.Namespace)
		{
			throw new XslTransformException("XmlWriter cannot process the sequence returned by the query, because it contains an attribute or namespace node.", string.Empty);
		}
		xwrt.NamespaceResolver = nsResolver;
		return xwrt;
	}

	public override void EndTree()
	{
		lastItemWasAtomic = false;
	}

	public override void WriteItem(XPathItem item)
	{
		if (item.IsNode)
		{
			XPathNavigator xPathNavigator = item as XPathNavigator;
			if (xPathNavigator.NodeType == XPathNodeType.Attribute || xPathNavigator.NodeType == XPathNodeType.Namespace)
			{
				throw new XslTransformException("XmlWriter cannot process the sequence returned by the query, because it contains an attribute or namespace node.", string.Empty);
			}
			CopyNode(xPathNavigator);
			lastItemWasAtomic = false;
		}
		else
		{
			WriteString(item.Value);
		}
	}

	private void WriteString(string value)
	{
		if (lastItemWasAtomic)
		{
			xwrt.WriteWhitespace(" ");
		}
		else
		{
			lastItemWasAtomic = true;
		}
		xwrt.WriteString(value);
	}

	private void CopyNode(XPathNavigator nav)
	{
		int num = 0;
		while (true)
		{
			if (CopyShallowNode(nav))
			{
				if (nav.NodeType == XPathNodeType.Element)
				{
					if (nav.MoveToFirstAttribute())
					{
						do
						{
							CopyShallowNode(nav);
						}
						while (nav.MoveToNextAttribute());
						nav.MoveToParent();
					}
					XPathNamespaceScope xPathNamespaceScope = ((num == 0) ? XPathNamespaceScope.ExcludeXml : XPathNamespaceScope.Local);
					if (nav.MoveToFirstNamespace(xPathNamespaceScope))
					{
						CopyNamespaces(nav, xPathNamespaceScope);
						nav.MoveToParent();
					}
					xwrt.StartElementContent();
				}
				if (nav.MoveToFirstChild())
				{
					num++;
					continue;
				}
				if (nav.NodeType == XPathNodeType.Element)
				{
					xwrt.WriteEndElement(nav.Prefix, nav.LocalName, nav.NamespaceURI);
				}
			}
			while (true)
			{
				if (num == 0)
				{
					return;
				}
				if (nav.MoveToNext())
				{
					break;
				}
				num--;
				nav.MoveToParent();
				if (nav.NodeType == XPathNodeType.Element)
				{
					xwrt.WriteFullEndElement(nav.Prefix, nav.LocalName, nav.NamespaceURI);
				}
			}
		}
	}

	private bool CopyShallowNode(XPathNavigator nav)
	{
		bool result = false;
		switch (nav.NodeType)
		{
		case XPathNodeType.Element:
			xwrt.WriteStartElement(nav.Prefix, nav.LocalName, nav.NamespaceURI);
			result = true;
			break;
		case XPathNodeType.Attribute:
			xwrt.WriteStartAttribute(nav.Prefix, nav.LocalName, nav.NamespaceURI);
			xwrt.WriteString(nav.Value);
			xwrt.WriteEndAttribute();
			break;
		case XPathNodeType.Text:
			xwrt.WriteString(nav.Value);
			break;
		case XPathNodeType.SignificantWhitespace:
		case XPathNodeType.Whitespace:
			xwrt.WriteWhitespace(nav.Value);
			break;
		case XPathNodeType.Root:
			result = true;
			break;
		case XPathNodeType.Comment:
			xwrt.WriteComment(nav.Value);
			break;
		case XPathNodeType.ProcessingInstruction:
			xwrt.WriteProcessingInstruction(nav.LocalName, nav.Value);
			break;
		case XPathNodeType.Namespace:
			xwrt.WriteNamespaceDeclaration(nav.LocalName, nav.Value);
			break;
		}
		return result;
	}

	private void CopyNamespaces(XPathNavigator nav, XPathNamespaceScope nsScope)
	{
		string localName = nav.LocalName;
		string value = nav.Value;
		if (nav.MoveToNextNamespace(nsScope))
		{
			CopyNamespaces(nav, nsScope);
		}
		xwrt.WriteNamespaceDeclaration(localName, value);
	}
}
