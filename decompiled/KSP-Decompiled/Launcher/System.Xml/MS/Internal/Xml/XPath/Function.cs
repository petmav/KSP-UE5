using System.Collections;
using System.Xml.XPath;

namespace MS.Internal.Xml.XPath;

internal class Function : AstNode
{
	public enum FunctionType
	{
		FuncLast,
		FuncPosition,
		FuncCount,
		FuncID,
		FuncLocalName,
		FuncNameSpaceUri,
		FuncName,
		FuncString,
		FuncBoolean,
		FuncNumber,
		FuncTrue,
		FuncFalse,
		FuncNot,
		FuncConcat,
		FuncStartsWith,
		FuncContains,
		FuncSubstringBefore,
		FuncSubstringAfter,
		FuncSubstring,
		FuncStringLength,
		FuncNormalize,
		FuncTranslate,
		FuncLang,
		FuncSum,
		FuncFloor,
		FuncCeiling,
		FuncRound,
		FuncUserDefined
	}

	private FunctionType functionType;

	private ArrayList argumentList;

	private string name;

	private string prefix;

	internal static XPathResultType[] ReturnTypes = new XPathResultType[28]
	{
		XPathResultType.Number,
		XPathResultType.Number,
		XPathResultType.Number,
		XPathResultType.NodeSet,
		XPathResultType.String,
		XPathResultType.String,
		XPathResultType.String,
		XPathResultType.String,
		XPathResultType.Boolean,
		XPathResultType.Number,
		XPathResultType.Boolean,
		XPathResultType.Boolean,
		XPathResultType.Boolean,
		XPathResultType.String,
		XPathResultType.Boolean,
		XPathResultType.Boolean,
		XPathResultType.String,
		XPathResultType.String,
		XPathResultType.String,
		XPathResultType.Number,
		XPathResultType.String,
		XPathResultType.String,
		XPathResultType.Boolean,
		XPathResultType.Number,
		XPathResultType.Number,
		XPathResultType.Number,
		XPathResultType.Number,
		XPathResultType.Any
	};

	public override AstType Type => AstType.Function;

	public override XPathResultType ReturnType => ReturnTypes[(int)functionType];

	public FunctionType TypeOfFunction => functionType;

	public ArrayList ArgumentList => argumentList;

	public string Prefix => prefix;

	public string Name => name;

	public Function(FunctionType ftype, ArrayList argumentList)
	{
		functionType = ftype;
		this.argumentList = new ArrayList(argumentList);
	}

	public Function(string prefix, string name, ArrayList argumentList)
	{
		functionType = FunctionType.FuncUserDefined;
		this.prefix = prefix;
		this.name = name;
		this.argumentList = new ArrayList(argumentList);
	}

	public Function(FunctionType ftype)
	{
		functionType = ftype;
	}

	public Function(FunctionType ftype, AstNode arg)
	{
		functionType = ftype;
		argumentList = new ArrayList();
		argumentList.Add(arg);
	}
}
