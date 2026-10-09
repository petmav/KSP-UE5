using System.Text;
using System.Xml;

namespace System.Security.Cryptography.Xml;

internal class CanonicalXmlAttribute : XmlAttribute, ICanonicalizableNode
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

	public CanonicalXmlAttribute(string prefix, string localName, string namespaceURI, XmlDocument doc, bool defaultNodeSetInclusionState)
		: base(prefix, localName, namespaceURI, doc)
	{
		IsInNodeSet = defaultNodeSetInclusionState;
	}

	public void Write(StringBuilder strBuilder, DocPosition docPos, AncestralNamespaceContextManager anc)
	{
		strBuilder.Append(" " + Name + "=\"");
		strBuilder.Append(Utils.EscapeAttributeValue(Value));
		strBuilder.Append("\"");
	}

	public void WriteHash(HashAlgorithm hash, DocPosition docPos, AncestralNamespaceContextManager anc)
	{
		UTF8Encoding uTF8Encoding = new UTF8Encoding(encoderShouldEmitUTF8Identifier: false);
		byte[] bytes = uTF8Encoding.GetBytes(" " + Name + "=\"");
		hash.TransformBlock(bytes, 0, bytes.Length, bytes, 0);
		bytes = uTF8Encoding.GetBytes(Utils.EscapeAttributeValue(Value));
		hash.TransformBlock(bytes, 0, bytes.Length, bytes, 0);
		bytes = uTF8Encoding.GetBytes("\"");
		hash.TransformBlock(bytes, 0, bytes.Length, bytes, 0);
	}
}
