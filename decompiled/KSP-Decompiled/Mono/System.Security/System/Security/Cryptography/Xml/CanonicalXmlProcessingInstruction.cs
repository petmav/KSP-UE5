using System.Text;
using System.Xml;

namespace System.Security.Cryptography.Xml;

internal class CanonicalXmlProcessingInstruction : XmlProcessingInstruction, ICanonicalizableNode
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

	public CanonicalXmlProcessingInstruction(string target, string data, XmlDocument doc, bool defaultNodeSetInclusionState)
		: base(target, data, doc)
	{
		_isInNodeSet = defaultNodeSetInclusionState;
	}

	public void Write(StringBuilder strBuilder, DocPosition docPos, AncestralNamespaceContextManager anc)
	{
		if (IsInNodeSet)
		{
			if (docPos == DocPosition.AfterRootElement)
			{
				strBuilder.Append('\n');
			}
			strBuilder.Append("<?");
			strBuilder.Append(Name);
			if (Value != null && Value.Length > 0)
			{
				strBuilder.Append(" " + Value);
			}
			strBuilder.Append("?>");
			if (docPos == DocPosition.BeforeRootElement)
			{
				strBuilder.Append('\n');
			}
		}
	}

	public void WriteHash(HashAlgorithm hash, DocPosition docPos, AncestralNamespaceContextManager anc)
	{
		if (IsInNodeSet)
		{
			UTF8Encoding uTF8Encoding = new UTF8Encoding(encoderShouldEmitUTF8Identifier: false);
			byte[] bytes;
			if (docPos == DocPosition.AfterRootElement)
			{
				bytes = uTF8Encoding.GetBytes("(char) 10");
				hash.TransformBlock(bytes, 0, bytes.Length, bytes, 0);
			}
			bytes = uTF8Encoding.GetBytes("<?");
			hash.TransformBlock(bytes, 0, bytes.Length, bytes, 0);
			bytes = uTF8Encoding.GetBytes(Name);
			hash.TransformBlock(bytes, 0, bytes.Length, bytes, 0);
			if (Value != null && Value.Length > 0)
			{
				bytes = uTF8Encoding.GetBytes(" " + Value);
				hash.TransformBlock(bytes, 0, bytes.Length, bytes, 0);
			}
			bytes = uTF8Encoding.GetBytes("?>");
			hash.TransformBlock(bytes, 0, bytes.Length, bytes, 0);
			if (docPos == DocPosition.BeforeRootElement)
			{
				bytes = uTF8Encoding.GetBytes("(char) 10");
				hash.TransformBlock(bytes, 0, bytes.Length, bytes, 0);
			}
		}
	}
}
