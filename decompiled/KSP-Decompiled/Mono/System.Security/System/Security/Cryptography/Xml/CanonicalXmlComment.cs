using System.Text;
using System.Xml;

namespace System.Security.Cryptography.Xml;

internal class CanonicalXmlComment : XmlComment, ICanonicalizableNode
{
	private bool _isInNodeSet;

	private bool _includeComments;

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

	public bool IncludeComments => _includeComments;

	public CanonicalXmlComment(string comment, XmlDocument doc, bool defaultNodeSetInclusionState, bool includeComments)
		: base(comment, doc)
	{
		_isInNodeSet = defaultNodeSetInclusionState;
		_includeComments = includeComments;
	}

	public void Write(StringBuilder strBuilder, DocPosition docPos, AncestralNamespaceContextManager anc)
	{
		if (IsInNodeSet && IncludeComments)
		{
			if (docPos == DocPosition.AfterRootElement)
			{
				strBuilder.Append('\n');
			}
			strBuilder.Append("<!--");
			strBuilder.Append(Value);
			strBuilder.Append("-->");
			if (docPos == DocPosition.BeforeRootElement)
			{
				strBuilder.Append('\n');
			}
		}
	}

	public void WriteHash(HashAlgorithm hash, DocPosition docPos, AncestralNamespaceContextManager anc)
	{
		if (IsInNodeSet && IncludeComments)
		{
			UTF8Encoding uTF8Encoding = new UTF8Encoding(encoderShouldEmitUTF8Identifier: false);
			byte[] bytes = uTF8Encoding.GetBytes("(char) 10");
			if (docPos == DocPosition.AfterRootElement)
			{
				hash.TransformBlock(bytes, 0, bytes.Length, bytes, 0);
			}
			bytes = uTF8Encoding.GetBytes("<!--");
			hash.TransformBlock(bytes, 0, bytes.Length, bytes, 0);
			bytes = uTF8Encoding.GetBytes(Value);
			hash.TransformBlock(bytes, 0, bytes.Length, bytes, 0);
			bytes = uTF8Encoding.GetBytes("-->");
			hash.TransformBlock(bytes, 0, bytes.Length, bytes, 0);
			if (docPos == DocPosition.BeforeRootElement)
			{
				bytes = uTF8Encoding.GetBytes("(char) 10");
				hash.TransformBlock(bytes, 0, bytes.Length, bytes, 0);
			}
		}
	}
}
