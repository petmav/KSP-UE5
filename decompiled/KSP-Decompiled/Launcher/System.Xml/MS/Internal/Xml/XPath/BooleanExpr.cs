using System.Xml;
using System.Xml.XPath;
using System.Xml.Xsl;

namespace MS.Internal.Xml.XPath;

internal sealed class BooleanExpr : ValueQuery
{
	private Query opnd1;

	private Query opnd2;

	private bool isOr;

	public override XPathResultType StaticType => XPathResultType.Boolean;

	public BooleanExpr(Operator.Op op, Query opnd1, Query opnd2)
	{
		if (opnd1.StaticType != XPathResultType.Boolean)
		{
			opnd1 = new BooleanFunctions(Function.FunctionType.FuncBoolean, opnd1);
		}
		if (opnd2.StaticType != XPathResultType.Boolean)
		{
			opnd2 = new BooleanFunctions(Function.FunctionType.FuncBoolean, opnd2);
		}
		this.opnd1 = opnd1;
		this.opnd2 = opnd2;
		isOr = op == Operator.Op.OR;
	}

	private BooleanExpr(BooleanExpr other)
		: base(other)
	{
		opnd1 = Query.Clone(other.opnd1);
		opnd2 = Query.Clone(other.opnd2);
		isOr = other.isOr;
	}

	public override void SetXsltContext(XsltContext context)
	{
		opnd1.SetXsltContext(context);
		opnd2.SetXsltContext(context);
	}

	public override object Evaluate(XPathNodeIterator nodeIterator)
	{
		object obj = opnd1.Evaluate(nodeIterator);
		if ((bool)obj == isOr)
		{
			return obj;
		}
		return opnd2.Evaluate(nodeIterator);
	}

	public override XPathNodeIterator Clone()
	{
		return new BooleanExpr(this);
	}

	public override void PrintQuery(XmlWriter w)
	{
		w.WriteStartElement(GetType().Name);
		w.WriteAttributeString("op", (isOr ? Operator.Op.OR : Operator.Op.AND).ToString());
		opnd1.PrintQuery(w);
		opnd2.PrintQuery(w);
		w.WriteEndElement();
	}
}
