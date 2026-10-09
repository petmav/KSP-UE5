using System;
using System.Xml;
using System.Xml.XPath;
using System.Xml.Xsl;

namespace MS.Internal.Xml.XPath;

internal sealed class BooleanFunctions : ValueQuery
{
	private Query arg;

	private Function.FunctionType funcType;

	public override XPathResultType StaticType => XPathResultType.Boolean;

	public BooleanFunctions(Function.FunctionType funcType, Query arg)
	{
		this.arg = arg;
		this.funcType = funcType;
	}

	private BooleanFunctions(BooleanFunctions other)
		: base(other)
	{
		arg = Query.Clone(other.arg);
		funcType = other.funcType;
	}

	public override void SetXsltContext(XsltContext context)
	{
		if (arg != null)
		{
			arg.SetXsltContext(context);
		}
	}

	public override object Evaluate(XPathNodeIterator nodeIterator)
	{
		return funcType switch
		{
			Function.FunctionType.FuncBoolean => toBoolean(nodeIterator), 
			Function.FunctionType.FuncNot => Not(nodeIterator), 
			Function.FunctionType.FuncTrue => true, 
			Function.FunctionType.FuncFalse => false, 
			Function.FunctionType.FuncLang => Lang(nodeIterator), 
			_ => false, 
		};
	}

	internal static bool toBoolean(double number)
	{
		if (number != 0.0)
		{
			return !double.IsNaN(number);
		}
		return false;
	}

	internal static bool toBoolean(string str)
	{
		return str.Length > 0;
	}

	internal bool toBoolean(XPathNodeIterator nodeIterator)
	{
		object obj = arg.Evaluate(nodeIterator);
		if (obj is XPathNodeIterator)
		{
			return arg.Advance() != null;
		}
		if (obj is string)
		{
			return toBoolean((string)obj);
		}
		if (obj is double)
		{
			return toBoolean((double)obj);
		}
		if (obj is bool)
		{
			return (bool)obj;
		}
		return true;
	}

	private bool Not(XPathNodeIterator nodeIterator)
	{
		return !(bool)arg.Evaluate(nodeIterator);
	}

	private bool Lang(XPathNodeIterator nodeIterator)
	{
		string text = arg.Evaluate(nodeIterator).ToString();
		string xmlLang = nodeIterator.Current.XmlLang;
		if (xmlLang.StartsWith(text, StringComparison.OrdinalIgnoreCase))
		{
			if (xmlLang.Length != text.Length)
			{
				return xmlLang[text.Length] == '-';
			}
			return true;
		}
		return false;
	}

	public override XPathNodeIterator Clone()
	{
		return new BooleanFunctions(this);
	}

	public override void PrintQuery(XmlWriter w)
	{
		w.WriteStartElement(GetType().Name);
		w.WriteAttributeString("name", funcType.ToString());
		if (arg != null)
		{
			arg.PrintQuery(w);
		}
		w.WriteEndElement();
	}
}
