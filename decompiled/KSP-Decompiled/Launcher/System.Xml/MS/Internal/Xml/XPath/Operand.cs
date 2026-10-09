using System.Xml.XPath;

namespace MS.Internal.Xml.XPath;

internal class Operand : AstNode
{
	private XPathResultType type;

	private object val;

	public override AstType Type => AstType.ConstantOperand;

	public override XPathResultType ReturnType => type;

	public object OperandValue => val;

	public Operand(string val)
	{
		type = XPathResultType.String;
		this.val = val;
	}

	public Operand(double val)
	{
		type = XPathResultType.Number;
		this.val = val;
	}

	public Operand(bool val)
	{
		type = XPathResultType.Boolean;
		this.val = val;
	}
}
