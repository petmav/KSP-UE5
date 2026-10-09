using System.Collections.Generic;
using System.Globalization;
using System.Text;
using System.Xml;
using System.Xml.XPath;
using System.Xml.Xsl;

namespace MS.Internal.Xml.XPath;

internal sealed class StringFunctions : ValueQuery
{
	private Function.FunctionType funcType;

	private IList<Query> argList;

	private static readonly CompareInfo compareInfo = CultureInfo.InvariantCulture.CompareInfo;

	public override XPathResultType StaticType
	{
		get
		{
			if (funcType == Function.FunctionType.FuncStringLength)
			{
				return XPathResultType.Number;
			}
			if (funcType == Function.FunctionType.FuncStartsWith || funcType == Function.FunctionType.FuncContains)
			{
				return XPathResultType.Boolean;
			}
			return XPathResultType.String;
		}
	}

	public StringFunctions(Function.FunctionType funcType, IList<Query> argList)
	{
		this.funcType = funcType;
		this.argList = argList;
	}

	private StringFunctions(StringFunctions other)
		: base(other)
	{
		funcType = other.funcType;
		Query[] array = new Query[other.argList.Count];
		for (int i = 0; i < array.Length; i++)
		{
			array[i] = Query.Clone(other.argList[i]);
		}
		argList = array;
	}

	public override void SetXsltContext(XsltContext context)
	{
		for (int i = 0; i < argList.Count; i++)
		{
			argList[i].SetXsltContext(context);
		}
	}

	public override object Evaluate(XPathNodeIterator nodeIterator)
	{
		return funcType switch
		{
			Function.FunctionType.FuncString => toString(nodeIterator), 
			Function.FunctionType.FuncConcat => Concat(nodeIterator), 
			Function.FunctionType.FuncStartsWith => StartsWith(nodeIterator), 
			Function.FunctionType.FuncContains => Contains(nodeIterator), 
			Function.FunctionType.FuncSubstringBefore => SubstringBefore(nodeIterator), 
			Function.FunctionType.FuncSubstringAfter => SubstringAfter(nodeIterator), 
			Function.FunctionType.FuncSubstring => Substring(nodeIterator), 
			Function.FunctionType.FuncStringLength => StringLength(nodeIterator), 
			Function.FunctionType.FuncNormalize => Normalize(nodeIterator), 
			Function.FunctionType.FuncTranslate => Translate(nodeIterator), 
			_ => string.Empty, 
		};
	}

	internal static string toString(double num)
	{
		return num.ToString("R", NumberFormatInfo.InvariantInfo);
	}

	internal static string toString(bool b)
	{
		if (!b)
		{
			return "false";
		}
		return "true";
	}

	private string toString(XPathNodeIterator nodeIterator)
	{
		if (argList.Count > 0)
		{
			object obj = argList[0].Evaluate(nodeIterator);
			switch (GetXPathType(obj))
			{
			case XPathResultType.NodeSet:
			{
				XPathNavigator xPathNavigator = argList[0].Advance();
				if (xPathNavigator == null)
				{
					return string.Empty;
				}
				return xPathNavigator.Value;
			}
			case XPathResultType.String:
				return (string)obj;
			case XPathResultType.Boolean:
				if (!(bool)obj)
				{
					return "false";
				}
				return "true";
			case (XPathResultType)4:
				return ((XPathNavigator)obj).Value;
			default:
				return toString((double)obj);
			}
		}
		return nodeIterator.Current.Value;
	}

	private string Concat(XPathNodeIterator nodeIterator)
	{
		int num = 0;
		StringBuilder stringBuilder = new StringBuilder();
		while (num < argList.Count)
		{
			stringBuilder.Append(argList[num++].Evaluate(nodeIterator).ToString());
		}
		return stringBuilder.ToString();
	}

	private bool StartsWith(XPathNodeIterator nodeIterator)
	{
		string text = argList[0].Evaluate(nodeIterator).ToString();
		string text2 = argList[1].Evaluate(nodeIterator).ToString();
		if (text.Length >= text2.Length)
		{
			return string.CompareOrdinal(text, 0, text2, 0, text2.Length) == 0;
		}
		return false;
	}

	private bool Contains(XPathNodeIterator nodeIterator)
	{
		string source = argList[0].Evaluate(nodeIterator).ToString();
		string value = argList[1].Evaluate(nodeIterator).ToString();
		return compareInfo.IndexOf(source, value, CompareOptions.Ordinal) >= 0;
	}

	private string SubstringBefore(XPathNodeIterator nodeIterator)
	{
		string text = argList[0].Evaluate(nodeIterator).ToString();
		string text2 = argList[1].Evaluate(nodeIterator).ToString();
		if (text2.Length == 0)
		{
			return text2;
		}
		int num = compareInfo.IndexOf(text, text2, CompareOptions.Ordinal);
		if (num >= 1)
		{
			return text.Substring(0, num);
		}
		return string.Empty;
	}

	private string SubstringAfter(XPathNodeIterator nodeIterator)
	{
		string text = argList[0].Evaluate(nodeIterator).ToString();
		string text2 = argList[1].Evaluate(nodeIterator).ToString();
		if (text2.Length == 0)
		{
			return text;
		}
		int num = compareInfo.IndexOf(text, text2, CompareOptions.Ordinal);
		if (num >= 0)
		{
			return text.Substring(num + text2.Length);
		}
		return string.Empty;
	}

	private string Substring(XPathNodeIterator nodeIterator)
	{
		string text = argList[0].Evaluate(nodeIterator).ToString();
		double num = XmlConvert.XPathRound(XmlConvert.ToXPathDouble(argList[1].Evaluate(nodeIterator))) - 1.0;
		if (double.IsNaN(num) || (double)text.Length <= num)
		{
			return string.Empty;
		}
		if (argList.Count == 3)
		{
			double num2 = XmlConvert.XPathRound(XmlConvert.ToXPathDouble(argList[2].Evaluate(nodeIterator)));
			if (double.IsNaN(num2))
			{
				return string.Empty;
			}
			if (num < 0.0 || num2 < 0.0)
			{
				num2 = num + num2;
				if (!(num2 > 0.0))
				{
					return string.Empty;
				}
				num = 0.0;
			}
			double num3 = (double)text.Length - num;
			if (num2 > num3)
			{
				num2 = num3;
			}
			return text.Substring((int)num, (int)num2);
		}
		if (num < 0.0)
		{
			num = 0.0;
		}
		return text.Substring((int)num);
	}

	private double StringLength(XPathNodeIterator nodeIterator)
	{
		if (argList.Count > 0)
		{
			return argList[0].Evaluate(nodeIterator).ToString().Length;
		}
		return nodeIterator.Current.Value.Length;
	}

	private string Normalize(XPathNodeIterator nodeIterator)
	{
		string value = ((argList.Count <= 0) ? nodeIterator.Current.Value : argList[0].Evaluate(nodeIterator).ToString());
		value = XmlConvert.TrimString(value);
		int i = 0;
		StringBuilder stringBuilder = new StringBuilder();
		bool flag = true;
		XmlCharType instance = XmlCharType.Instance;
		for (; i < value.Length; i++)
		{
			if (!instance.IsWhiteSpace(value[i]))
			{
				flag = true;
				stringBuilder.Append(value[i]);
			}
			else if (flag)
			{
				flag = false;
				stringBuilder.Append(' ');
			}
		}
		return stringBuilder.ToString();
	}

	private string Translate(XPathNodeIterator nodeIterator)
	{
		string text = argList[0].Evaluate(nodeIterator).ToString();
		string text2 = argList[1].Evaluate(nodeIterator).ToString();
		string text3 = argList[2].Evaluate(nodeIterator).ToString();
		int i = 0;
		StringBuilder stringBuilder = new StringBuilder();
		for (; i < text.Length; i++)
		{
			int num = text2.IndexOf(text[i]);
			if (num != -1)
			{
				if (num < text3.Length)
				{
					stringBuilder.Append(text3[num]);
				}
			}
			else
			{
				stringBuilder.Append(text[i]);
			}
		}
		return stringBuilder.ToString();
	}

	public override XPathNodeIterator Clone()
	{
		return new StringFunctions(this);
	}

	public override void PrintQuery(XmlWriter w)
	{
		w.WriteStartElement(GetType().Name);
		w.WriteAttributeString("name", funcType.ToString());
		foreach (Query arg in argList)
		{
			arg.PrintQuery(w);
		}
		w.WriteEndElement();
	}
}
