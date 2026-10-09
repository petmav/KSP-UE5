using System.Xml.XPath;

namespace MS.Internal.Xml.XPath;

internal class Operator : AstNode
{
	public enum Op
	{
		INVALID,
		OR,
		AND,
		EQ,
		NE,
		LT,
		LE,
		GT,
		GE,
		PLUS,
		MINUS,
		MUL,
		DIV,
		MOD,
		UNION
	}

	private static Op[] invertOp = new Op[9]
	{
		Op.INVALID,
		Op.INVALID,
		Op.INVALID,
		Op.EQ,
		Op.NE,
		Op.GT,
		Op.GE,
		Op.LT,
		Op.LE
	};

	private Op opType;

	private AstNode opnd1;

	private AstNode opnd2;

	public override AstType Type => AstType.Operator;

	public override XPathResultType ReturnType
	{
		get
		{
			if (opType <= Op.GE)
			{
				return XPathResultType.Boolean;
			}
			if (opType <= Op.MOD)
			{
				return XPathResultType.Number;
			}
			return XPathResultType.NodeSet;
		}
	}

	public Op OperatorType => opType;

	public AstNode Operand1 => opnd1;

	public AstNode Operand2 => opnd2;

	public static Op InvertOperator(Op op)
	{
		return invertOp[(int)op];
	}

	public Operator(Op op, AstNode opnd1, AstNode opnd2)
	{
		opType = op;
		this.opnd1 = opnd1;
		this.opnd2 = opnd2;
	}
}
