using System.Xml.XPath;

namespace MS.Internal.Xml.XPath;

internal class Filter : AstNode
{
	private AstNode input;

	private AstNode condition;

	public override AstType Type => AstType.Filter;

	public override XPathResultType ReturnType => XPathResultType.NodeSet;

	public AstNode Input => input;

	public AstNode Condition => condition;

	public Filter(AstNode input, AstNode condition)
	{
		this.input = input;
		this.condition = condition;
	}
}
