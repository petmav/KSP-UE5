using System.Text;
using System.Xml;

namespace System.Security.Cryptography.Xml;

internal class CanonicalXmlSignificantWhitespace : XmlSignificantWhitespace, ICanonicalizableNode
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

	public CanonicalXmlSignificantWhitespace(string strData, XmlDocument doc, bool defaultNodeSetInclusionState)
		: base(strData, doc)
	{
		_isInNodeSet = defaultNodeSetInclusionState;
	}

	public void Write(StringBuilder strBuilder, DocPosition docPos, AncestralNamespaceContextManager anc)
	{
		if (IsInNodeSet && docPos == DocPosition.InRootElement)
		{
			strBuilder.Append(Utils.EscapeWhitespaceData(Value));
		}
	}

	public void WriteHash(HashAlgorithm hash, DocPosition docPos, AncestralNamespaceContextManager anc)
	{
		if (IsInNodeSet && docPos == DocPosition.InRootElement)
		{
			byte[] bytes = new UTF8Encoding(encoderShouldEmitUTF8Identifier: false).GetBytes(Utils.EscapeWhitespaceData(Value));
			hash.TransformBlock(bytes, 0, bytes.Length, bytes, 0);
		}
	}
}
