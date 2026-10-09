using System;
using System.Xml;
using System.Xml.XPath;
using System.Xml.Xsl;

namespace MS.Internal.Xml.XPath;

internal sealed class NumberFunctions : ValueQuery
{
	private Query arg;

	private Function.FunctionType ftype;

	public override XPathResultType StaticType => XPathResultType.Number;

	public NumberFunctions(Function.FunctionType ftype, Query arg)
	{
		this.arg = arg;
		this.ftype = ftype;
	}

	private NumberFunctions(NumberFunctions other)
		: base(other)
	{
		arg = Query.Clone(other.arg);
		ftype = other.ftype;
	}

	public override void SetXsltContext(XsltContext context)
	{
		if (arg != null)
		{
			arg.SetXsltContext(context);
		}
	}

	internal static double Number(bool arg)
	{
		if (!arg)
		{
			return 0.0;
		}
		return 1.0;
	}

	internal static double Number(string arg)
	{
		return XmlConvert.ToXPathDouble(arg);
	}

	public override object Evaluate(XPathNodeIterator nodeIterator)
	{
		return ftype switch
		{
			Function.FunctionType.FuncNumber => Number(nodeIterator), 
			Function.FunctionType.FuncSum => Sum(nodeIterator), 
			Function.FunctionType.FuncFloor => Floor(nodeIterator), 
			Function.FunctionType.FuncCeiling => Ceiling(nodeIterator), 
			Function.FunctionType.FuncRound => Round(nodeIterator), 
			_ => null, 
		};
	}

	private double Number(XPathNodeIterator nodeIterator)
	{
		if (arg == null)
		{
			return XmlConvert.ToXPathDouble(nodeIterator.Current.Value);
		}
		object obj = arg.Evaluate(nodeIterator);
		switch (GetXPathType(obj))
		{
		case XPathResultType.NodeSet:
		{
			XPathNavigator xPathNavigator = arg.Advance();
			if (xPathNavigator != null)
			{
				return Number(xPathNavigator.Value);
			}
			break;
		}
		case XPathResultType.String:
			return Number((string)obj);
		case XPathResultType.Boolean:
			return Number((bool)obj);
		case XPathResultType.Number:
			return (double)obj;
		case (XPathResultType)4:
			return Number(((XPathNavigator)obj).Value);
		}
		return double.NaN;
	}

	private double Sum(XPathNodeIterator nodeIterator)
	{
		double num = 0.0;
		arg.Evaluate(nodeIterator);
		XPathNavigator xPathNavigator;
		while ((xPathNavigator = arg.Advance()) != null)
		{
			num += Number(xPathNavigator.Value);
		}
		return num;
	}

	private double Floor(XPathNodeIterator nodeIterator)
	{
		return Math.Floor((double)arg.Evaluate(nodeIterator));
	}

	private double Ceiling(XPathNodeIterator nodeIterator)
	{
		return Math.Ceiling((double)arg.Evaluate(nodeIterator));
	}

	private double Round(XPathNodeIterator nodeIterator)
	{
		return XmlConvert.XPathRound(XmlConvert.ToXPathDouble(arg.Evaluate(nodeIterator)));
	}

	public override XPathNodeIterator Clone()
	{
		return new NumberFunctions(this);
	}

	public override void PrintQuery(XmlWriter w)
	{
		w.WriteStartElement(GetType().Name);
		w.WriteAttributeString("name", ftype.ToString());
		if (arg != null)
		{
			arg.PrintQuery(w);
		}
		w.WriteEndElement();
	}
}
