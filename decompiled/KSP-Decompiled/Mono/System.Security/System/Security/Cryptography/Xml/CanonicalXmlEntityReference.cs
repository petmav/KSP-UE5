using System.Text;
using System.Xml;

namespace System.Security.Cryptography.Xml;

internal class CanonicalXmlEntityReference : XmlEntityReference, ICanonicalizableNode
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

	public CanonicalXmlEntityReference(string name, XmlDocument doc, bool defaultNodeSetInclusionState)
		: base(name, doc)
	{
		_isInNodeSet = defaultNodeSetInclusionState;
	}

	public void Write(StringBuilder strBuilder, DocPosition docPos, AncestralNamespaceContextManager anc)
	{
		if (IsInNodeSet)
		{
			CanonicalizationDispatcher.WriteGenericNode(this, strBuilder, docPos, anc);
		}
	}

	public void WriteHash(HashAlgorithm hash, DocPosition docPos, AncestralNamespaceContextManager anc)
	{
		if (IsInNodeSet)
		{
			CanonicalizationDispatcher.WriteHashGenericNode(this, hash, docPos, anc);
		}
	}
}
