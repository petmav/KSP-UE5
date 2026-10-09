using System.Collections.Generic;
using System.Xml.Xsl.Qil;

namespace System.Xml.Xsl.Xslt;

internal class XslNode
{
	public readonly XslNodeType NodeType;

	public ISourceLineInfo SourceLine;

	public NsDecl Namespaces;

	public readonly QilName Name;

	public readonly object Arg;

	public readonly XslVersion XslVersion;

	public XslFlags Flags;

	private List<XslNode> content;

	private static readonly IList<XslNode> EmptyList = new List<XslNode>().AsReadOnly();

	public string Select => (string)Arg;

	public bool ForwardsCompatible => XslVersion == XslVersion.ForwardsCompatible;

	public IList<XslNode> Content
	{
		get
		{
			IList<XslNode> list = content;
			return list ?? EmptyList;
		}
	}

	internal string TraceName => null;

	public XslNode(XslNodeType nodeType, QilName name, object arg, XslVersion xslVer)
	{
		NodeType = nodeType;
		Name = name;
		Arg = arg;
		XslVersion = xslVer;
	}

	public XslNode(XslNodeType nodeType)
	{
		NodeType = nodeType;
		XslVersion = XslVersion.Version10;
	}

	public void SetContent(List<XslNode> content)
	{
		this.content = content;
	}

	public void AddContent(XslNode node)
	{
		if (content == null)
		{
			content = new List<XslNode>();
		}
		content.Add(node);
	}

	public void InsertContent(IEnumerable<XslNode> collection)
	{
		if (content == null)
		{
			content = new List<XslNode>(collection);
		}
		else
		{
			content.InsertRange(0, collection);
		}
	}
}
