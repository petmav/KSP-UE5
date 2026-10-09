using System.Text;
using System.Xml.XPath;

namespace MS.Internal.Xml.Cache;

internal sealed class XPathNodeInfoAtom
{
	private string localName;

	private string namespaceUri;

	private string prefix;

	private string baseUri;

	private XPathNode[] pageParent;

	private XPathNode[] pageSibling;

	private XPathNode[] pageSimilar;

	private XPathDocument doc;

	private int lineNumBase;

	private int linePosBase;

	private int hashCode;

	private int localNameHash;

	private XPathNodeInfoAtom next;

	private XPathNodePageInfo pageInfo;

	public XPathNodePageInfo PageInfo => pageInfo;

	public string LocalName => localName;

	public string NamespaceUri => namespaceUri;

	public string Prefix => prefix;

	public string BaseUri => baseUri;

	public XPathNode[] SiblingPage => pageSibling;

	public XPathNode[] SimilarElementPage => pageSimilar;

	public XPathNode[] ParentPage => pageParent;

	public XPathDocument Document => doc;

	public int LineNumberBase => lineNumBase;

	public int LinePositionBase => linePosBase;

	public int LocalNameHashCode => localNameHash;

	public XPathNodeInfoAtom Next
	{
		get
		{
			return next;
		}
		set
		{
			next = value;
		}
	}

	public XPathNodeInfoAtom(XPathNodePageInfo pageInfo)
	{
		this.pageInfo = pageInfo;
	}

	public XPathNodeInfoAtom(string localName, string namespaceUri, string prefix, string baseUri, XPathNode[] pageParent, XPathNode[] pageSibling, XPathNode[] pageSimilar, XPathDocument doc, int lineNumBase, int linePosBase)
	{
		Init(localName, namespaceUri, prefix, baseUri, pageParent, pageSibling, pageSimilar, doc, lineNumBase, linePosBase);
	}

	public void Init(string localName, string namespaceUri, string prefix, string baseUri, XPathNode[] pageParent, XPathNode[] pageSibling, XPathNode[] pageSimilar, XPathDocument doc, int lineNumBase, int linePosBase)
	{
		this.localName = localName;
		this.namespaceUri = namespaceUri;
		this.prefix = prefix;
		this.baseUri = baseUri;
		this.pageParent = pageParent;
		this.pageSibling = pageSibling;
		this.pageSimilar = pageSimilar;
		this.doc = doc;
		this.lineNumBase = lineNumBase;
		this.linePosBase = linePosBase;
		next = null;
		pageInfo = null;
		hashCode = 0;
		localNameHash = 0;
		for (int i = 0; i < this.localName.Length; i++)
		{
			localNameHash += (localNameHash << 7) ^ this.localName[i];
		}
	}

	public override int GetHashCode()
	{
		if (hashCode == 0)
		{
			int num = localNameHash;
			if (pageSibling != null)
			{
				num += (num << 7) ^ pageSibling[0].PageInfo.PageNumber;
			}
			if (pageParent != null)
			{
				num += (num << 7) ^ pageParent[0].PageInfo.PageNumber;
			}
			if (pageSimilar != null)
			{
				num += (num << 7) ^ pageSimilar[0].PageInfo.PageNumber;
			}
			hashCode = ((num == 0) ? 1 : num);
		}
		return hashCode;
	}

	public override bool Equals(object other)
	{
		XPathNodeInfoAtom xPathNodeInfoAtom = other as XPathNodeInfoAtom;
		if (GetHashCode() == xPathNodeInfoAtom.GetHashCode() && (object)localName == xPathNodeInfoAtom.localName && pageSibling == xPathNodeInfoAtom.pageSibling && (object)namespaceUri == xPathNodeInfoAtom.namespaceUri && pageParent == xPathNodeInfoAtom.pageParent && pageSimilar == xPathNodeInfoAtom.pageSimilar && (object)prefix == xPathNodeInfoAtom.prefix && (object)baseUri == xPathNodeInfoAtom.baseUri && lineNumBase == xPathNodeInfoAtom.lineNumBase && linePosBase == xPathNodeInfoAtom.linePosBase)
		{
			return true;
		}
		return false;
	}

	public override string ToString()
	{
		StringBuilder stringBuilder = new StringBuilder();
		stringBuilder.Append("hash=");
		stringBuilder.Append(GetHashCode());
		stringBuilder.Append(", ");
		if (localName.Length != 0)
		{
			stringBuilder.Append('{');
			stringBuilder.Append(namespaceUri);
			stringBuilder.Append('}');
			if (prefix.Length != 0)
			{
				stringBuilder.Append(prefix);
				stringBuilder.Append(':');
			}
			stringBuilder.Append(localName);
			stringBuilder.Append(", ");
		}
		if (pageParent != null)
		{
			stringBuilder.Append("parent=");
			stringBuilder.Append(pageParent[0].PageInfo.PageNumber);
			stringBuilder.Append(", ");
		}
		if (pageSibling != null)
		{
			stringBuilder.Append("sibling=");
			stringBuilder.Append(pageSibling[0].PageInfo.PageNumber);
			stringBuilder.Append(", ");
		}
		if (pageSimilar != null)
		{
			stringBuilder.Append("similar=");
			stringBuilder.Append(pageSimilar[0].PageInfo.PageNumber);
			stringBuilder.Append(", ");
		}
		stringBuilder.Append("lineNum=");
		stringBuilder.Append(lineNumBase);
		stringBuilder.Append(", ");
		stringBuilder.Append("linePos=");
		stringBuilder.Append(linePosBase);
		return stringBuilder.ToString();
	}
}
