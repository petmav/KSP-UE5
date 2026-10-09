using System.Threading;
using System.Xml.Linq;
using System.Xml.Schema;

namespace System.Xml.XPath;

internal class XNodeNavigator : XPathNavigator, IXmlLineInfo
{
	private const int DocumentContentMask = 386;

	private static readonly int[] ElementContentMasks = new int[10] { 0, 2, 0, 0, 24, 0, 0, 128, 256, 410 };

	private new const int TextMask = 24;

	private static XAttribute XmlNamespaceDeclaration;

	private object source;

	private XElement parent;

	private XmlNameTable nameTable;

	public override string BaseURI
	{
		get
		{
			if (source is XObject xObject)
			{
				return xObject.BaseUri;
			}
			if (parent != null)
			{
				return parent.BaseUri;
			}
			return string.Empty;
		}
	}

	public override bool HasAttributes
	{
		get
		{
			if (source is XElement { lastAttr: { } xAttribute } xElement)
			{
				do
				{
					xAttribute = xAttribute.next;
					if (!xAttribute.IsNamespaceDeclaration)
					{
						return true;
					}
				}
				while (xAttribute != xElement.lastAttr);
			}
			return false;
		}
	}

	public override bool HasChildren
	{
		get
		{
			if (source is XContainer { content: not null } xContainer)
			{
				XNode xNode = xContainer.content as XNode;
				if (xNode != null)
				{
					do
					{
						xNode = xNode.next;
						if (IsContent(xContainer, xNode))
						{
							return true;
						}
					}
					while (xNode != xContainer.content);
					return false;
				}
				if (((string)xContainer.content).Length != 0 && (xContainer.parent != null || xContainer is XElement))
				{
					return true;
				}
			}
			return false;
		}
	}

	public override bool IsEmptyElement
	{
		get
		{
			if (source is XElement xElement)
			{
				return xElement.IsEmpty;
			}
			return false;
		}
	}

	public override string LocalName => nameTable.Add(GetLocalName());

	public override string Name
	{
		get
		{
			string prefix = GetPrefix();
			if (prefix.Length == 0)
			{
				return nameTable.Add(GetLocalName());
			}
			return nameTable.Add(prefix + ":" + GetLocalName());
		}
	}

	public override string NamespaceURI => nameTable.Add(GetNamespaceURI());

	public override XmlNameTable NameTable => nameTable;

	public override XPathNodeType NodeType
	{
		get
		{
			if (source is XObject xObject)
			{
				switch (xObject.NodeType)
				{
				case XmlNodeType.Element:
					return XPathNodeType.Element;
				case XmlNodeType.Attribute:
					if (parent != null)
					{
						return XPathNodeType.Namespace;
					}
					return XPathNodeType.Attribute;
				case XmlNodeType.Document:
					return XPathNodeType.Root;
				case XmlNodeType.Comment:
					return XPathNodeType.Comment;
				case XmlNodeType.ProcessingInstruction:
					return XPathNodeType.ProcessingInstruction;
				default:
					return XPathNodeType.Text;
				}
			}
			return XPathNodeType.Text;
		}
	}

	public override string Prefix => nameTable.Add(GetPrefix());

	public override object UnderlyingObject
	{
		get
		{
			if (source is string)
			{
				source = parent.LastNode;
				parent = null;
			}
			return source;
		}
	}

	public override string Value
	{
		get
		{
			if (source is XObject xObject)
			{
				switch (xObject.NodeType)
				{
				case XmlNodeType.Element:
					return ((XElement)xObject).Value;
				case XmlNodeType.Attribute:
					return ((XAttribute)xObject).Value;
				case XmlNodeType.Document:
				{
					XElement root = ((XDocument)xObject).Root;
					if (root == null)
					{
						return string.Empty;
					}
					return root.Value;
				}
				case XmlNodeType.Text:
				case XmlNodeType.CDATA:
					return CollectText((XText)xObject);
				case XmlNodeType.Comment:
					return ((XComment)xObject).Value;
				case XmlNodeType.ProcessingInstruction:
					return ((XProcessingInstruction)xObject).Data;
				default:
					return string.Empty;
				}
			}
			return (string)source;
		}
	}

	int IXmlLineInfo.LineNumber
	{
		get
		{
			if (source is IXmlLineInfo xmlLineInfo)
			{
				return xmlLineInfo.LineNumber;
			}
			return 0;
		}
	}

	int IXmlLineInfo.LinePosition
	{
		get
		{
			if (source is IXmlLineInfo xmlLineInfo)
			{
				return xmlLineInfo.LinePosition;
			}
			return 0;
		}
	}

	public XNodeNavigator(XNode node, XmlNameTable nameTable)
	{
		source = node;
		this.nameTable = ((nameTable != null) ? nameTable : CreateNameTable());
	}

	public XNodeNavigator(XNodeNavigator other)
	{
		source = other.source;
		parent = other.parent;
		nameTable = other.nameTable;
	}

	private string GetLocalName()
	{
		if (source is XElement xElement)
		{
			return xElement.Name.LocalName;
		}
		if (source is XAttribute xAttribute)
		{
			if (parent != null && xAttribute.Name.NamespaceName.Length == 0)
			{
				return string.Empty;
			}
			return xAttribute.Name.LocalName;
		}
		if (source is XProcessingInstruction xProcessingInstruction)
		{
			return xProcessingInstruction.Target;
		}
		return string.Empty;
	}

	private string GetNamespaceURI()
	{
		if (source is XElement xElement)
		{
			return xElement.Name.NamespaceName;
		}
		if (source is XAttribute xAttribute)
		{
			if (parent != null)
			{
				return string.Empty;
			}
			return xAttribute.Name.NamespaceName;
		}
		return string.Empty;
	}

	private string GetPrefix()
	{
		if (source is XElement xElement)
		{
			string prefixOfNamespace = xElement.GetPrefixOfNamespace(xElement.Name.Namespace);
			if (prefixOfNamespace != null)
			{
				return prefixOfNamespace;
			}
			return string.Empty;
		}
		if (source is XAttribute xAttribute)
		{
			if (parent != null)
			{
				return string.Empty;
			}
			string prefixOfNamespace2 = xAttribute.GetPrefixOfNamespace(xAttribute.Name.Namespace);
			if (prefixOfNamespace2 != null)
			{
				return prefixOfNamespace2;
			}
		}
		return string.Empty;
	}

	public override bool CheckValidity(XmlSchemaSet schemas, ValidationEventHandler validationEventHandler)
	{
		throw new NotSupportedException(System.Xml.Linq.Res.GetString("NotSupported_CheckValidity"));
	}

	public override XPathNavigator Clone()
	{
		return new XNodeNavigator(this);
	}

	public override bool IsSamePosition(XPathNavigator navigator)
	{
		if (!(navigator is XNodeNavigator n))
		{
			return false;
		}
		return IsSamePosition(this, n);
	}

	public override bool MoveTo(XPathNavigator navigator)
	{
		if (navigator is XNodeNavigator xNodeNavigator)
		{
			source = xNodeNavigator.source;
			parent = xNodeNavigator.parent;
			return true;
		}
		return false;
	}

	public override bool MoveToAttribute(string localName, string namespaceName)
	{
		if (source is XElement { lastAttr: { } xAttribute } xElement)
		{
			do
			{
				xAttribute = xAttribute.next;
				if (xAttribute.Name.LocalName == localName && xAttribute.Name.NamespaceName == namespaceName && !xAttribute.IsNamespaceDeclaration)
				{
					source = xAttribute;
					return true;
				}
			}
			while (xAttribute != xElement.lastAttr);
		}
		return false;
	}

	public override bool MoveToChild(string localName, string namespaceName)
	{
		if (source is XContainer { content: not null } xContainer)
		{
			XNode xNode = xContainer.content as XNode;
			if (xNode != null)
			{
				do
				{
					xNode = xNode.next;
					if (xNode is XElement xElement && xElement.Name.LocalName == localName && xElement.Name.NamespaceName == namespaceName)
					{
						source = xElement;
						return true;
					}
				}
				while (xNode != xContainer.content);
			}
		}
		return false;
	}

	public override bool MoveToChild(XPathNodeType type)
	{
		if (source is XContainer { content: not null } xContainer)
		{
			XNode xNode = xContainer.content as XNode;
			if (xNode != null)
			{
				int num = GetElementContentMask(type);
				if ((0x18u & (uint)num) != 0 && xContainer.parent == null && xContainer is XDocument)
				{
					num &= -25;
				}
				do
				{
					xNode = xNode.next;
					if (((1 << (int)xNode.NodeType) & num) != 0)
					{
						source = xNode;
						return true;
					}
				}
				while (xNode != xContainer.content);
				return false;
			}
			string text = (string)xContainer.content;
			if (text.Length != 0)
			{
				int elementContentMask = GetElementContentMask(type);
				if ((0x18u & (uint)elementContentMask) != 0 && xContainer.parent == null && xContainer is XDocument)
				{
					return false;
				}
				if ((8u & (uint)elementContentMask) != 0)
				{
					source = text;
					parent = (XElement)xContainer;
					return true;
				}
			}
		}
		return false;
	}

	public override bool MoveToFirstAttribute()
	{
		if (source is XElement { lastAttr: { } xAttribute } xElement)
		{
			do
			{
				xAttribute = xAttribute.next;
				if (!xAttribute.IsNamespaceDeclaration)
				{
					source = xAttribute;
					return true;
				}
			}
			while (xAttribute != xElement.lastAttr);
		}
		return false;
	}

	public override bool MoveToFirstChild()
	{
		if (source is XContainer { content: not null } xContainer)
		{
			XNode xNode = xContainer.content as XNode;
			if (xNode != null)
			{
				do
				{
					xNode = xNode.next;
					if (IsContent(xContainer, xNode))
					{
						source = xNode;
						return true;
					}
				}
				while (xNode != xContainer.content);
				return false;
			}
			string text = (string)xContainer.content;
			if (text.Length != 0 && (xContainer.parent != null || xContainer is XElement))
			{
				source = text;
				parent = (XElement)xContainer;
				return true;
			}
		}
		return false;
	}

	public override bool MoveToFirstNamespace(XPathNamespaceScope scope)
	{
		if (source is XElement e)
		{
			XAttribute xAttribute = null;
			switch (scope)
			{
			case XPathNamespaceScope.Local:
				xAttribute = GetFirstNamespaceDeclarationLocal(e);
				break;
			case XPathNamespaceScope.ExcludeXml:
				xAttribute = GetFirstNamespaceDeclarationGlobal(e);
				while (xAttribute != null && xAttribute.Name.LocalName == "xml")
				{
					xAttribute = GetNextNamespaceDeclarationGlobal(xAttribute);
				}
				break;
			case XPathNamespaceScope.All:
				xAttribute = GetFirstNamespaceDeclarationGlobal(e);
				if (xAttribute == null)
				{
					xAttribute = GetXmlNamespaceDeclaration();
				}
				break;
			}
			if (xAttribute != null)
			{
				source = xAttribute;
				parent = e;
				return true;
			}
		}
		return false;
	}

	public override bool MoveToId(string id)
	{
		throw new NotSupportedException(System.Xml.Linq.Res.GetString("NotSupported_MoveToId"));
	}

	public override bool MoveToNamespace(string localName)
	{
		if (source is XElement e)
		{
			if (localName == "xmlns")
			{
				return false;
			}
			if (localName != null && localName.Length == 0)
			{
				localName = "xmlns";
			}
			for (XAttribute xAttribute = GetFirstNamespaceDeclarationGlobal(e); xAttribute != null; xAttribute = GetNextNamespaceDeclarationGlobal(xAttribute))
			{
				if (xAttribute.Name.LocalName == localName)
				{
					source = xAttribute;
					parent = e;
					return true;
				}
			}
			if (localName == "xml")
			{
				source = GetXmlNamespaceDeclaration();
				parent = e;
				return true;
			}
		}
		return false;
	}

	public override bool MoveToNext()
	{
		XNode xNode = source as XNode;
		if (xNode != null)
		{
			XContainer xContainer = xNode.parent;
			if (xContainer != null && xNode != xContainer.content)
			{
				do
				{
					XNode next = xNode.next;
					if (IsContent(xContainer, next) && (!(xNode is XText) || !(next is XText)))
					{
						source = next;
						return true;
					}
					xNode = next;
				}
				while (xNode != xContainer.content);
			}
		}
		return false;
	}

	public override bool MoveToNext(string localName, string namespaceName)
	{
		XNode xNode = source as XNode;
		if (xNode != null)
		{
			XContainer xContainer = xNode.parent;
			if (xContainer != null && xNode != xContainer.content)
			{
				do
				{
					xNode = xNode.next;
					if (xNode is XElement xElement && xElement.Name.LocalName == localName && xElement.Name.NamespaceName == namespaceName)
					{
						source = xElement;
						return true;
					}
				}
				while (xNode != xContainer.content);
			}
		}
		return false;
	}

	public override bool MoveToNext(XPathNodeType type)
	{
		XNode xNode = source as XNode;
		if (xNode != null)
		{
			XContainer xContainer = xNode.parent;
			if (xContainer != null && xNode != xContainer.content)
			{
				int num = GetElementContentMask(type);
				if ((0x18u & (uint)num) != 0 && xContainer.parent == null && xContainer is XDocument)
				{
					num &= -25;
				}
				do
				{
					XNode next = xNode.next;
					if (((1 << (int)next.NodeType) & num) != 0 && (!(xNode is XText) || !(next is XText)))
					{
						source = next;
						return true;
					}
					xNode = next;
				}
				while (xNode != xContainer.content);
			}
		}
		return false;
	}

	public override bool MoveToNextAttribute()
	{
		XAttribute xAttribute = source as XAttribute;
		if (xAttribute != null && parent == null)
		{
			XElement xElement = (XElement)xAttribute.parent;
			if (xElement != null)
			{
				while (xAttribute != xElement.lastAttr)
				{
					xAttribute = xAttribute.next;
					if (!xAttribute.IsNamespaceDeclaration)
					{
						source = xAttribute;
						return true;
					}
				}
			}
		}
		return false;
	}

	public override bool MoveToNextNamespace(XPathNamespaceScope scope)
	{
		XAttribute xAttribute = source as XAttribute;
		if (xAttribute != null && parent != null && !IsXmlNamespaceDeclaration(xAttribute))
		{
			switch (scope)
			{
			case XPathNamespaceScope.Local:
				if (xAttribute.parent != parent)
				{
					return false;
				}
				xAttribute = GetNextNamespaceDeclarationLocal(xAttribute);
				break;
			case XPathNamespaceScope.ExcludeXml:
				do
				{
					xAttribute = GetNextNamespaceDeclarationGlobal(xAttribute);
				}
				while (xAttribute != null && (xAttribute.Name.LocalName == "xml" || HasNamespaceDeclarationInScope(xAttribute, parent)));
				break;
			case XPathNamespaceScope.All:
				do
				{
					xAttribute = GetNextNamespaceDeclarationGlobal(xAttribute);
				}
				while (xAttribute != null && HasNamespaceDeclarationInScope(xAttribute, parent));
				if (xAttribute == null && !HasNamespaceDeclarationInScope(GetXmlNamespaceDeclaration(), parent))
				{
					xAttribute = GetXmlNamespaceDeclaration();
				}
				break;
			}
			if (xAttribute != null)
			{
				source = xAttribute;
				return true;
			}
		}
		return false;
	}

	public override bool MoveToParent()
	{
		if (parent != null)
		{
			source = parent;
			parent = null;
			return true;
		}
		XObject xObject = (XObject)source;
		if (xObject.parent != null)
		{
			source = xObject.parent;
			return true;
		}
		return false;
	}

	public override bool MoveToPrevious()
	{
		if (source is XNode { parent: { } xContainer } xNode)
		{
			XNode xNode2 = (XNode)xContainer.content;
			if (xNode2.next != xNode)
			{
				XNode xNode3 = null;
				do
				{
					xNode2 = xNode2.next;
					if (IsContent(xContainer, xNode2))
					{
						xNode3 = ((xNode3 is XText && xNode2 is XText) ? xNode3 : xNode2);
					}
				}
				while (xNode2.next != xNode);
				if (xNode3 != null)
				{
					source = xNode3;
					return true;
				}
			}
		}
		return false;
	}

	public override XmlReader ReadSubtree()
	{
		if (!(source is XContainer node))
		{
			throw new InvalidOperationException(System.Xml.Linq.Res.GetString("InvalidOperation_BadNodeType", NodeType));
		}
		return new XNodeReader(node, nameTable);
	}

	bool IXmlLineInfo.HasLineInfo()
	{
		if (source is IXmlLineInfo xmlLineInfo)
		{
			return xmlLineInfo.HasLineInfo();
		}
		return false;
	}

	private static string CollectText(XText n)
	{
		string text = n.Value;
		if (n.parent != null)
		{
			while (n != n.parent.content)
			{
				n = n.next as XText;
				if (n == null)
				{
					break;
				}
				text += n.Value;
			}
		}
		return text;
	}

	private static XmlNameTable CreateNameTable()
	{
		NameTable obj = new NameTable();
		obj.Add(string.Empty);
		obj.Add("http://www.w3.org/2000/xmlns/");
		obj.Add("http://www.w3.org/XML/1998/namespace");
		return obj;
	}

	private static bool IsContent(XContainer c, XNode n)
	{
		if (c.parent != null || c is XElement)
		{
			return true;
		}
		return ((1 << (int)n.NodeType) & 0x182) != 0;
	}

	private static bool IsSamePosition(XNodeNavigator n1, XNodeNavigator n2)
	{
		if (n1.source == n2.source && n1.parent == n2.parent)
		{
			return true;
		}
		if ((n1.parent != null) ^ (n2.parent != null))
		{
			if (n1.source is XText xText)
			{
				if (xText.Value == n2.source)
				{
					return xText.parent == n2.parent;
				}
				return false;
			}
			if (n2.source is XText xText2)
			{
				if (xText2.Value == n1.source)
				{
					return xText2.parent == n1.parent;
				}
				return false;
			}
		}
		return false;
	}

	private static bool IsXmlNamespaceDeclaration(XAttribute a)
	{
		return a == GetXmlNamespaceDeclaration();
	}

	private static int GetElementContentMask(XPathNodeType type)
	{
		return ElementContentMasks[(int)type];
	}

	private static XAttribute GetFirstNamespaceDeclarationGlobal(XElement e)
	{
		do
		{
			XAttribute firstNamespaceDeclarationLocal = GetFirstNamespaceDeclarationLocal(e);
			if (firstNamespaceDeclarationLocal != null)
			{
				return firstNamespaceDeclarationLocal;
			}
			e = e.parent as XElement;
		}
		while (e != null);
		return null;
	}

	private static XAttribute GetFirstNamespaceDeclarationLocal(XElement e)
	{
		XAttribute xAttribute = e.lastAttr;
		if (xAttribute != null)
		{
			do
			{
				xAttribute = xAttribute.next;
				if (xAttribute.IsNamespaceDeclaration)
				{
					return xAttribute;
				}
			}
			while (xAttribute != e.lastAttr);
		}
		return null;
	}

	private static XAttribute GetNextNamespaceDeclarationGlobal(XAttribute a)
	{
		XElement xElement = (XElement)a.parent;
		if (xElement == null)
		{
			return null;
		}
		XAttribute nextNamespaceDeclarationLocal = GetNextNamespaceDeclarationLocal(a);
		if (nextNamespaceDeclarationLocal != null)
		{
			return nextNamespaceDeclarationLocal;
		}
		if (!(xElement.parent is XElement e))
		{
			return null;
		}
		return GetFirstNamespaceDeclarationGlobal(e);
	}

	private static XAttribute GetNextNamespaceDeclarationLocal(XAttribute a)
	{
		XElement xElement = (XElement)a.parent;
		if (xElement == null)
		{
			return null;
		}
		while (a != xElement.lastAttr)
		{
			a = a.next;
			if (a.IsNamespaceDeclaration)
			{
				return a;
			}
		}
		return null;
	}

	private static XAttribute GetXmlNamespaceDeclaration()
	{
		if (XmlNamespaceDeclaration == null)
		{
			Interlocked.CompareExchange(ref XmlNamespaceDeclaration, new XAttribute(XNamespace.Xmlns.GetName("xml"), "http://www.w3.org/XML/1998/namespace"), null);
		}
		return XmlNamespaceDeclaration;
	}

	private static bool HasNamespaceDeclarationInScope(XAttribute a, XElement e)
	{
		XName name = a.Name;
		while (e != null && e != a.parent)
		{
			if (e.Attribute(name) != null)
			{
				return true;
			}
			e = e.parent as XElement;
		}
		return false;
	}
}
