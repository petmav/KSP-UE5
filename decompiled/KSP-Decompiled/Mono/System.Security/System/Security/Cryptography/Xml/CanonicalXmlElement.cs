using System.Collections;
using System.Text;
using System.Xml;

namespace System.Security.Cryptography.Xml;

internal class CanonicalXmlElement : XmlElement, ICanonicalizableNode
{
	private bool _isInNodeSet;

	public bool IsInNodeSet
	{
		get
		{
			return _isInNodeSet;
		}
		set
		{
			_isInNodeSet = value;
		}
	}

	public CanonicalXmlElement(string prefix, string localName, string namespaceURI, XmlDocument doc, bool defaultNodeSetInclusionState)
		: base(prefix, localName, namespaceURI, doc)
	{
		_isInNodeSet = defaultNodeSetInclusionState;
	}

	public void Write(StringBuilder strBuilder, DocPosition docPos, AncestralNamespaceContextManager anc)
	{
		Hashtable nsLocallyDeclared = new Hashtable();
		SortedList sortedList = new SortedList(new NamespaceSortOrder());
		SortedList sortedList2 = new SortedList(new AttributeSortOrder());
		XmlAttributeCollection xmlAttributeCollection = Attributes;
		if (xmlAttributeCollection != null)
		{
			foreach (XmlAttribute item in xmlAttributeCollection)
			{
				if (((CanonicalXmlAttribute)item).IsInNodeSet || Utils.IsNamespaceNode(item) || Utils.IsXmlNamespaceNode(item))
				{
					if (Utils.IsNamespaceNode(item))
					{
						anc.TrackNamespaceNode(item, sortedList, nsLocallyDeclared);
					}
					else if (Utils.IsXmlNamespaceNode(item))
					{
						anc.TrackXmlNamespaceNode(item, sortedList, sortedList2, nsLocallyDeclared);
					}
					else if (IsInNodeSet)
					{
						sortedList2.Add(item, null);
					}
				}
			}
		}
		if (!Utils.IsCommittedNamespace(this, Prefix, NamespaceURI))
		{
			string text = ((Prefix.Length > 0) ? ("xmlns:" + Prefix) : "xmlns");
			XmlAttribute xmlAttribute2 = OwnerDocument.CreateAttribute(text);
			xmlAttribute2.Value = NamespaceURI;
			anc.TrackNamespaceNode(xmlAttribute2, sortedList, nsLocallyDeclared);
		}
		if (IsInNodeSet)
		{
			anc.GetNamespacesToRender(this, sortedList2, sortedList, nsLocallyDeclared);
			strBuilder.Append("<" + Name);
			foreach (object key in sortedList.GetKeyList())
			{
				(key as CanonicalXmlAttribute).Write(strBuilder, docPos, anc);
			}
			foreach (object key2 in sortedList2.GetKeyList())
			{
				(key2 as CanonicalXmlAttribute).Write(strBuilder, docPos, anc);
			}
			strBuilder.Append(">");
		}
		anc.EnterElementContext();
		anc.LoadUnrenderedNamespaces(nsLocallyDeclared);
		anc.LoadRenderedNamespaces(sortedList);
		foreach (XmlNode childNode in ChildNodes)
		{
			CanonicalizationDispatcher.Write(childNode, strBuilder, docPos, anc);
		}
		anc.ExitElementContext();
		if (IsInNodeSet)
		{
			strBuilder.Append("</" + Name + ">");
		}
	}

	public void WriteHash(HashAlgorithm hash, DocPosition docPos, AncestralNamespaceContextManager anc)
	{
		Hashtable nsLocallyDeclared = new Hashtable();
		SortedList sortedList = new SortedList(new NamespaceSortOrder());
		SortedList sortedList2 = new SortedList(new AttributeSortOrder());
		UTF8Encoding uTF8Encoding = new UTF8Encoding(encoderShouldEmitUTF8Identifier: false);
		XmlAttributeCollection xmlAttributeCollection = Attributes;
		if (xmlAttributeCollection != null)
		{
			foreach (XmlAttribute item in xmlAttributeCollection)
			{
				if (((CanonicalXmlAttribute)item).IsInNodeSet || Utils.IsNamespaceNode(item) || Utils.IsXmlNamespaceNode(item))
				{
					if (Utils.IsNamespaceNode(item))
					{
						anc.TrackNamespaceNode(item, sortedList, nsLocallyDeclared);
					}
					else if (Utils.IsXmlNamespaceNode(item))
					{
						anc.TrackXmlNamespaceNode(item, sortedList, sortedList2, nsLocallyDeclared);
					}
					else if (IsInNodeSet)
					{
						sortedList2.Add(item, null);
					}
				}
			}
		}
		if (!Utils.IsCommittedNamespace(this, Prefix, NamespaceURI))
		{
			string text = ((Prefix.Length > 0) ? ("xmlns:" + Prefix) : "xmlns");
			XmlAttribute xmlAttribute2 = OwnerDocument.CreateAttribute(text);
			xmlAttribute2.Value = NamespaceURI;
			anc.TrackNamespaceNode(xmlAttribute2, sortedList, nsLocallyDeclared);
		}
		if (IsInNodeSet)
		{
			anc.GetNamespacesToRender(this, sortedList2, sortedList, nsLocallyDeclared);
			byte[] bytes = uTF8Encoding.GetBytes("<" + Name);
			hash.TransformBlock(bytes, 0, bytes.Length, bytes, 0);
			foreach (object key in sortedList.GetKeyList())
			{
				(key as CanonicalXmlAttribute).WriteHash(hash, docPos, anc);
			}
			foreach (object key2 in sortedList2.GetKeyList())
			{
				(key2 as CanonicalXmlAttribute).WriteHash(hash, docPos, anc);
			}
			bytes = uTF8Encoding.GetBytes(">");
			hash.TransformBlock(bytes, 0, bytes.Length, bytes, 0);
		}
		anc.EnterElementContext();
		anc.LoadUnrenderedNamespaces(nsLocallyDeclared);
		anc.LoadRenderedNamespaces(sortedList);
		foreach (XmlNode childNode in ChildNodes)
		{
			CanonicalizationDispatcher.WriteHash(childNode, hash, docPos, anc);
		}
		anc.ExitElementContext();
		if (IsInNodeSet)
		{
			byte[] bytes = uTF8Encoding.GetBytes("</" + Name + ">");
			hash.TransformBlock(bytes, 0, bytes.Length, bytes, 0);
		}
	}
}
