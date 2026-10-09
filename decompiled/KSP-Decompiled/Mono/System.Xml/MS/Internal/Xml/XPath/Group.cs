using System.Xml.XPath;

namespace MS.Internal.Xml.XPath;

internal class Group : AstNode
{
	private AstNode groupNode;

	public override AstType Type => AstType.Group;

	public override XPathResultType ReturnType => XPathResultType.NodeSet;

	public AstNode GroupNode => groupNode;

	public Group(AstNode groupNode)
	{
		this.groupNode = groupNode;
	}
}
