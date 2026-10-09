using System.Collections;
using System.Xml.XPath;

namespace MS.Internal.Xml.XPath;

internal class XPathParser
{
	private class ParamInfo
	{
		private Function.FunctionType ftype;

		private int minargs;

		private int maxargs;

		private XPathResultType[] argTypes;

		public Function.FunctionType FType => ftype;

		public int Minargs => minargs;

		public int Maxargs => maxargs;

		public XPathResultType[] ArgTypes => argTypes;

		internal ParamInfo(Function.FunctionType ftype, int minargs, int maxargs, XPathResultType[] argTypes)
		{
			this.ftype = ftype;
			this.minargs = minargs;
			this.maxargs = maxargs;
			this.argTypes = argTypes;
		}
	}

	private XPathScanner scanner;

	private int parseDepth;

	private const int MaxParseDepth = 200;

	private static readonly XPathResultType[] temparray1 = new XPathResultType[0];

	private static readonly XPathResultType[] temparray2 = new XPathResultType[1] { XPathResultType.NodeSet };

	private static readonly XPathResultType[] temparray3 = new XPathResultType[1] { XPathResultType.Any };

	private static readonly XPathResultType[] temparray4 = new XPathResultType[1] { XPathResultType.String };

	private static readonly XPathResultType[] temparray5 = new XPathResultType[2]
	{
		XPathResultType.String,
		XPathResultType.String
	};

	private static readonly XPathResultType[] temparray6 = new XPathResultType[3]
	{
		XPathResultType.String,
		XPathResultType.Number,
		XPathResultType.Number
	};

	private static readonly XPathResultType[] temparray7 = new XPathResultType[3]
	{
		XPathResultType.String,
		XPathResultType.String,
		XPathResultType.String
	};

	private static readonly XPathResultType[] temparray8 = new XPathResultType[1] { XPathResultType.Boolean };

	private static readonly XPathResultType[] temparray9 = new XPathResultType[1];

	private static Hashtable functionTable = CreateFunctionTable();

	private static Hashtable AxesTable = CreateAxesTable();

	private XPathParser(XPathScanner scanner)
	{
		this.scanner = scanner;
	}

	public static AstNode ParseXPathExpresion(string xpathExpresion)
	{
		XPathScanner xPathScanner = new XPathScanner(xpathExpresion);
		AstNode result = new XPathParser(xPathScanner).ParseExpresion(null);
		if (xPathScanner.Kind != XPathScanner.LexKind.Eof)
		{
			throw XPathException.Create("'{0}' has an invalid token.", xPathScanner.SourceText);
		}
		return result;
	}

	public static AstNode ParseXPathPattern(string xpathPattern)
	{
		XPathScanner xPathScanner = new XPathScanner(xpathPattern);
		AstNode result = new XPathParser(xPathScanner).ParsePattern(null);
		if (xPathScanner.Kind != XPathScanner.LexKind.Eof)
		{
			throw XPathException.Create("'{0}' has an invalid token.", xPathScanner.SourceText);
		}
		return result;
	}

	private AstNode ParseExpresion(AstNode qyInput)
	{
		if (++parseDepth > 200)
		{
			throw XPathException.Create("The xpath query is too complex.");
		}
		AstNode result = ParseOrExpr(qyInput);
		parseDepth--;
		return result;
	}

	private AstNode ParseOrExpr(AstNode qyInput)
	{
		AstNode astNode = ParseAndExpr(qyInput);
		while (TestOp("or"))
		{
			NextLex();
			astNode = new Operator(Operator.Op.OR, astNode, ParseAndExpr(qyInput));
		}
		return astNode;
	}

	private AstNode ParseAndExpr(AstNode qyInput)
	{
		AstNode astNode = ParseEqualityExpr(qyInput);
		while (TestOp("and"))
		{
			NextLex();
			astNode = new Operator(Operator.Op.AND, astNode, ParseEqualityExpr(qyInput));
		}
		return astNode;
	}

	private AstNode ParseEqualityExpr(AstNode qyInput)
	{
		AstNode astNode = ParseRelationalExpr(qyInput);
		while (true)
		{
			Operator.Op op = ((scanner.Kind == XPathScanner.LexKind.Eq) ? Operator.Op.EQ : ((scanner.Kind == XPathScanner.LexKind.Ne) ? Operator.Op.NE : Operator.Op.INVALID));
			if (op == Operator.Op.INVALID)
			{
				break;
			}
			NextLex();
			astNode = new Operator(op, astNode, ParseRelationalExpr(qyInput));
		}
		return astNode;
	}

	private AstNode ParseRelationalExpr(AstNode qyInput)
	{
		AstNode astNode = ParseAdditiveExpr(qyInput);
		while (true)
		{
			Operator.Op op = ((scanner.Kind == XPathScanner.LexKind.Lt) ? Operator.Op.LT : ((scanner.Kind == XPathScanner.LexKind.Le) ? Operator.Op.LE : ((scanner.Kind == XPathScanner.LexKind.Gt) ? Operator.Op.GT : ((scanner.Kind == XPathScanner.LexKind.Ge) ? Operator.Op.GE : Operator.Op.INVALID))));
			if (op == Operator.Op.INVALID)
			{
				break;
			}
			NextLex();
			astNode = new Operator(op, astNode, ParseAdditiveExpr(qyInput));
		}
		return astNode;
	}

	private AstNode ParseAdditiveExpr(AstNode qyInput)
	{
		AstNode astNode = ParseMultiplicativeExpr(qyInput);
		while (true)
		{
			Operator.Op op = ((scanner.Kind == XPathScanner.LexKind.Plus) ? Operator.Op.PLUS : ((scanner.Kind == XPathScanner.LexKind.Minus) ? Operator.Op.MINUS : Operator.Op.INVALID));
			if (op == Operator.Op.INVALID)
			{
				break;
			}
			NextLex();
			astNode = new Operator(op, astNode, ParseMultiplicativeExpr(qyInput));
		}
		return astNode;
	}

	private AstNode ParseMultiplicativeExpr(AstNode qyInput)
	{
		AstNode astNode = ParseUnaryExpr(qyInput);
		while (true)
		{
			Operator.Op op = ((scanner.Kind == XPathScanner.LexKind.Star) ? Operator.Op.MUL : (TestOp("div") ? Operator.Op.DIV : (TestOp("mod") ? Operator.Op.MOD : Operator.Op.INVALID)));
			if (op == Operator.Op.INVALID)
			{
				break;
			}
			NextLex();
			astNode = new Operator(op, astNode, ParseUnaryExpr(qyInput));
		}
		return astNode;
	}

	private AstNode ParseUnaryExpr(AstNode qyInput)
	{
		bool flag = false;
		while (scanner.Kind == XPathScanner.LexKind.Minus)
		{
			NextLex();
			flag = !flag;
		}
		if (flag)
		{
			return new Operator(Operator.Op.MUL, ParseUnionExpr(qyInput), new Operand(-1.0));
		}
		return ParseUnionExpr(qyInput);
	}

	private AstNode ParseUnionExpr(AstNode qyInput)
	{
		AstNode astNode = ParsePathExpr(qyInput);
		while (scanner.Kind == XPathScanner.LexKind.Union)
		{
			NextLex();
			AstNode astNode2 = ParsePathExpr(qyInput);
			CheckNodeSet(astNode.ReturnType);
			CheckNodeSet(astNode2.ReturnType);
			astNode = new Operator(Operator.Op.UNION, astNode, astNode2);
		}
		return astNode;
	}

	private static bool IsNodeType(XPathScanner scaner)
	{
		if (scaner.Prefix.Length == 0)
		{
			if (!(scaner.Name == "node") && !(scaner.Name == "text") && !(scaner.Name == "processing-instruction"))
			{
				return scaner.Name == "comment";
			}
			return true;
		}
		return false;
	}

	private AstNode ParsePathExpr(AstNode qyInput)
	{
		AstNode astNode;
		if (IsPrimaryExpr(scanner))
		{
			astNode = ParseFilterExpr(qyInput);
			if (scanner.Kind == XPathScanner.LexKind.Slash)
			{
				NextLex();
				astNode = ParseRelativeLocationPath(astNode);
			}
			else if (scanner.Kind == XPathScanner.LexKind.SlashSlash)
			{
				NextLex();
				astNode = ParseRelativeLocationPath(new Axis(Axis.AxisType.DescendantOrSelf, astNode));
			}
		}
		else
		{
			astNode = ParseLocationPath(null);
		}
		return astNode;
	}

	private AstNode ParseFilterExpr(AstNode qyInput)
	{
		AstNode astNode = ParsePrimaryExpr(qyInput);
		while (scanner.Kind == XPathScanner.LexKind.LBracket)
		{
			astNode = new Filter(astNode, ParsePredicate(astNode));
		}
		return astNode;
	}

	private AstNode ParsePredicate(AstNode qyInput)
	{
		CheckNodeSet(qyInput.ReturnType);
		PassToken(XPathScanner.LexKind.LBracket);
		AstNode result = ParseExpresion(qyInput);
		PassToken(XPathScanner.LexKind.RBracket);
		return result;
	}

	private AstNode ParseLocationPath(AstNode qyInput)
	{
		if (scanner.Kind == XPathScanner.LexKind.Slash)
		{
			NextLex();
			AstNode astNode = new Root();
			if (IsStep(scanner.Kind))
			{
				astNode = ParseRelativeLocationPath(astNode);
			}
			return astNode;
		}
		if (scanner.Kind == XPathScanner.LexKind.SlashSlash)
		{
			NextLex();
			return ParseRelativeLocationPath(new Axis(Axis.AxisType.DescendantOrSelf, new Root()));
		}
		return ParseRelativeLocationPath(qyInput);
	}

	private AstNode ParseRelativeLocationPath(AstNode qyInput)
	{
		AstNode astNode = qyInput;
		while (true)
		{
			astNode = ParseStep(astNode);
			if (XPathScanner.LexKind.SlashSlash == scanner.Kind)
			{
				NextLex();
				astNode = new Axis(Axis.AxisType.DescendantOrSelf, astNode);
				continue;
			}
			if (XPathScanner.LexKind.Slash != scanner.Kind)
			{
				break;
			}
			NextLex();
		}
		return astNode;
	}

	private static bool IsStep(XPathScanner.LexKind lexKind)
	{
		if (lexKind != XPathScanner.LexKind.Dot && lexKind != XPathScanner.LexKind.DotDot && lexKind != XPathScanner.LexKind.At && lexKind != XPathScanner.LexKind.Axe && lexKind != XPathScanner.LexKind.Star)
		{
			return lexKind == XPathScanner.LexKind.Name;
		}
		return true;
	}

	private AstNode ParseStep(AstNode qyInput)
	{
		AstNode astNode;
		if (XPathScanner.LexKind.Dot == scanner.Kind)
		{
			NextLex();
			astNode = new Axis(Axis.AxisType.Self, qyInput);
		}
		else if (XPathScanner.LexKind.DotDot == scanner.Kind)
		{
			NextLex();
			astNode = new Axis(Axis.AxisType.Parent, qyInput);
		}
		else
		{
			Axis.AxisType axisType = Axis.AxisType.Child;
			switch (scanner.Kind)
			{
			case XPathScanner.LexKind.At:
				axisType = Axis.AxisType.Attribute;
				NextLex();
				break;
			case XPathScanner.LexKind.Axe:
				axisType = GetAxis(scanner);
				NextLex();
				break;
			}
			XPathNodeType nodeType = ((axisType != Axis.AxisType.Attribute) ? XPathNodeType.Element : XPathNodeType.Attribute);
			astNode = ParseNodeTest(qyInput, axisType, nodeType);
			while (XPathScanner.LexKind.LBracket == scanner.Kind)
			{
				astNode = new Filter(astNode, ParsePredicate(astNode));
			}
		}
		return astNode;
	}

	private AstNode ParseNodeTest(AstNode qyInput, Axis.AxisType axisType, XPathNodeType nodeType)
	{
		string prefix;
		string text;
		switch (scanner.Kind)
		{
		case XPathScanner.LexKind.Name:
			if (scanner.CanBeFunction && IsNodeType(scanner))
			{
				prefix = string.Empty;
				text = string.Empty;
				nodeType = ((scanner.Name == "comment") ? XPathNodeType.Comment : ((scanner.Name == "text") ? XPathNodeType.Text : ((scanner.Name == "node") ? XPathNodeType.All : ((scanner.Name == "processing-instruction") ? XPathNodeType.ProcessingInstruction : XPathNodeType.Root))));
				NextLex();
				PassToken(XPathScanner.LexKind.LParens);
				if (nodeType == XPathNodeType.ProcessingInstruction && scanner.Kind != XPathScanner.LexKind.RParens)
				{
					CheckToken(XPathScanner.LexKind.String);
					text = scanner.StringValue;
					NextLex();
				}
				PassToken(XPathScanner.LexKind.RParens);
			}
			else
			{
				prefix = scanner.Prefix;
				text = scanner.Name;
				NextLex();
				if (text == "*")
				{
					text = string.Empty;
				}
			}
			break;
		case XPathScanner.LexKind.Star:
			prefix = string.Empty;
			text = string.Empty;
			NextLex();
			break;
		default:
			throw XPathException.Create("Expression must evaluate to a node-set.", scanner.SourceText);
		}
		return new Axis(axisType, qyInput, prefix, text, nodeType);
	}

	private static bool IsPrimaryExpr(XPathScanner scanner)
	{
		if (scanner.Kind != XPathScanner.LexKind.String && scanner.Kind != XPathScanner.LexKind.Number && scanner.Kind != XPathScanner.LexKind.Dollar && scanner.Kind != XPathScanner.LexKind.LParens)
		{
			if (scanner.Kind == XPathScanner.LexKind.Name && scanner.CanBeFunction)
			{
				return !IsNodeType(scanner);
			}
			return false;
		}
		return true;
	}

	private AstNode ParsePrimaryExpr(AstNode qyInput)
	{
		AstNode astNode = null;
		switch (scanner.Kind)
		{
		case XPathScanner.LexKind.String:
			astNode = new Operand(scanner.StringValue);
			NextLex();
			break;
		case XPathScanner.LexKind.Number:
			astNode = new Operand(scanner.NumberValue);
			NextLex();
			break;
		case XPathScanner.LexKind.Dollar:
			NextLex();
			CheckToken(XPathScanner.LexKind.Name);
			astNode = new Variable(scanner.Name, scanner.Prefix);
			NextLex();
			break;
		case XPathScanner.LexKind.LParens:
			NextLex();
			astNode = ParseExpresion(qyInput);
			if (astNode.Type != AstNode.AstType.ConstantOperand)
			{
				astNode = new Group(astNode);
			}
			PassToken(XPathScanner.LexKind.RParens);
			break;
		case XPathScanner.LexKind.Name:
			if (scanner.CanBeFunction && !IsNodeType(scanner))
			{
				astNode = ParseMethod(null);
			}
			break;
		}
		return astNode;
	}

	private AstNode ParseMethod(AstNode qyInput)
	{
		ArrayList arrayList = new ArrayList();
		string name = scanner.Name;
		string prefix = scanner.Prefix;
		PassToken(XPathScanner.LexKind.Name);
		PassToken(XPathScanner.LexKind.LParens);
		if (scanner.Kind != XPathScanner.LexKind.RParens)
		{
			while (true)
			{
				arrayList.Add(ParseExpresion(qyInput));
				if (scanner.Kind == XPathScanner.LexKind.RParens)
				{
					break;
				}
				PassToken(XPathScanner.LexKind.Comma);
			}
		}
		PassToken(XPathScanner.LexKind.RParens);
		if (prefix.Length == 0)
		{
			ParamInfo paramInfo = (ParamInfo)functionTable[name];
			if (paramInfo != null)
			{
				int num = arrayList.Count;
				if (num < paramInfo.Minargs)
				{
					throw XPathException.Create("Function '{0}' in '{1}' has an invalid number of arguments.", name, scanner.SourceText);
				}
				if (paramInfo.FType == Function.FunctionType.FuncConcat)
				{
					for (int i = 0; i < num; i++)
					{
						AstNode astNode = (AstNode)arrayList[i];
						if (astNode.ReturnType != XPathResultType.String)
						{
							astNode = new Function(Function.FunctionType.FuncString, astNode);
						}
						arrayList[i] = astNode;
					}
				}
				else
				{
					if (paramInfo.Maxargs < num)
					{
						throw XPathException.Create("Function '{0}' in '{1}' has an invalid number of arguments.", name, scanner.SourceText);
					}
					if (paramInfo.ArgTypes.Length < num)
					{
						num = paramInfo.ArgTypes.Length;
					}
					for (int j = 0; j < num; j++)
					{
						AstNode astNode2 = (AstNode)arrayList[j];
						if (paramInfo.ArgTypes[j] == XPathResultType.Any || paramInfo.ArgTypes[j] == astNode2.ReturnType)
						{
							continue;
						}
						switch (paramInfo.ArgTypes[j])
						{
						case XPathResultType.NodeSet:
							if (!(astNode2 is Variable) && (!(astNode2 is Function) || astNode2.ReturnType != XPathResultType.Any))
							{
								throw XPathException.Create("The argument to function '{0}' in '{1}' cannot be converted to a node-set.", name, scanner.SourceText);
							}
							break;
						case XPathResultType.String:
							astNode2 = new Function(Function.FunctionType.FuncString, astNode2);
							break;
						case XPathResultType.Number:
							astNode2 = new Function(Function.FunctionType.FuncNumber, astNode2);
							break;
						case XPathResultType.Boolean:
							astNode2 = new Function(Function.FunctionType.FuncBoolean, astNode2);
							break;
						}
						arrayList[j] = astNode2;
					}
				}
				return new Function(paramInfo.FType, arrayList);
			}
		}
		return new Function(prefix, name, arrayList);
	}

	private AstNode ParsePattern(AstNode qyInput)
	{
		AstNode astNode = ParseLocationPathPattern(qyInput);
		while (scanner.Kind == XPathScanner.LexKind.Union)
		{
			NextLex();
			astNode = new Operator(Operator.Op.UNION, astNode, ParseLocationPathPattern(qyInput));
		}
		return astNode;
	}

	private AstNode ParseLocationPathPattern(AstNode qyInput)
	{
		AstNode astNode = null;
		switch (scanner.Kind)
		{
		case XPathScanner.LexKind.Slash:
			NextLex();
			astNode = new Root();
			if (scanner.Kind == XPathScanner.LexKind.Eof || scanner.Kind == XPathScanner.LexKind.Union)
			{
				return astNode;
			}
			break;
		case XPathScanner.LexKind.SlashSlash:
			NextLex();
			astNode = new Axis(Axis.AxisType.DescendantOrSelf, new Root());
			break;
		case XPathScanner.LexKind.Name:
			if (!scanner.CanBeFunction)
			{
				break;
			}
			astNode = ParseIdKeyPattern(qyInput);
			if (astNode != null)
			{
				switch (scanner.Kind)
				{
				case XPathScanner.LexKind.Slash:
					NextLex();
					break;
				case XPathScanner.LexKind.SlashSlash:
					NextLex();
					astNode = new Axis(Axis.AxisType.DescendantOrSelf, astNode);
					break;
				default:
					return astNode;
				}
			}
			break;
		}
		return ParseRelativePathPattern(astNode);
	}

	private AstNode ParseIdKeyPattern(AstNode qyInput)
	{
		ArrayList arrayList = new ArrayList();
		if (scanner.Prefix.Length == 0)
		{
			if (scanner.Name == "id")
			{
				ParamInfo obj = (ParamInfo)functionTable["id"];
				NextLex();
				PassToken(XPathScanner.LexKind.LParens);
				CheckToken(XPathScanner.LexKind.String);
				arrayList.Add(new Operand(scanner.StringValue));
				NextLex();
				PassToken(XPathScanner.LexKind.RParens);
				return new Function(obj.FType, arrayList);
			}
			if (scanner.Name == "key")
			{
				NextLex();
				PassToken(XPathScanner.LexKind.LParens);
				CheckToken(XPathScanner.LexKind.String);
				arrayList.Add(new Operand(scanner.StringValue));
				NextLex();
				PassToken(XPathScanner.LexKind.Comma);
				CheckToken(XPathScanner.LexKind.String);
				arrayList.Add(new Operand(scanner.StringValue));
				NextLex();
				PassToken(XPathScanner.LexKind.RParens);
				return new Function("", "key", arrayList);
			}
		}
		return null;
	}

	private AstNode ParseRelativePathPattern(AstNode qyInput)
	{
		AstNode astNode = ParseStepPattern(qyInput);
		if (XPathScanner.LexKind.SlashSlash == scanner.Kind)
		{
			NextLex();
			astNode = ParseRelativePathPattern(new Axis(Axis.AxisType.DescendantOrSelf, astNode));
		}
		else if (XPathScanner.LexKind.Slash == scanner.Kind)
		{
			NextLex();
			astNode = ParseRelativePathPattern(astNode);
		}
		return astNode;
	}

	private AstNode ParseStepPattern(AstNode qyInput)
	{
		Axis.AxisType axisType = Axis.AxisType.Child;
		switch (scanner.Kind)
		{
		case XPathScanner.LexKind.At:
			axisType = Axis.AxisType.Attribute;
			NextLex();
			break;
		case XPathScanner.LexKind.Axe:
			axisType = GetAxis(scanner);
			if (axisType != Axis.AxisType.Child && axisType != Axis.AxisType.Attribute)
			{
				throw XPathException.Create("'{0}' has an invalid token.", scanner.SourceText);
			}
			NextLex();
			break;
		}
		XPathNodeType nodeType = ((axisType != Axis.AxisType.Attribute) ? XPathNodeType.Element : XPathNodeType.Attribute);
		AstNode astNode = ParseNodeTest(qyInput, axisType, nodeType);
		while (XPathScanner.LexKind.LBracket == scanner.Kind)
		{
			astNode = new Filter(astNode, ParsePredicate(astNode));
		}
		return astNode;
	}

	private void CheckToken(XPathScanner.LexKind t)
	{
		if (scanner.Kind != t)
		{
			throw XPathException.Create("'{0}' has an invalid token.", scanner.SourceText);
		}
	}

	private void PassToken(XPathScanner.LexKind t)
	{
		CheckToken(t);
		NextLex();
	}

	private void NextLex()
	{
		scanner.NextLex();
	}

	private bool TestOp(string op)
	{
		if (scanner.Kind == XPathScanner.LexKind.Name && scanner.Prefix.Length == 0)
		{
			return scanner.Name.Equals(op);
		}
		return false;
	}

	private void CheckNodeSet(XPathResultType t)
	{
		if (t != XPathResultType.NodeSet && t != XPathResultType.Any)
		{
			throw XPathException.Create("Expression must evaluate to a node-set.", scanner.SourceText);
		}
	}

	private static Hashtable CreateFunctionTable()
	{
		return new Hashtable(36)
		{
			{
				"last",
				new ParamInfo(Function.FunctionType.FuncLast, 0, 0, temparray1)
			},
			{
				"position",
				new ParamInfo(Function.FunctionType.FuncPosition, 0, 0, temparray1)
			},
			{
				"name",
				new ParamInfo(Function.FunctionType.FuncName, 0, 1, temparray2)
			},
			{
				"namespace-uri",
				new ParamInfo(Function.FunctionType.FuncNameSpaceUri, 0, 1, temparray2)
			},
			{
				"local-name",
				new ParamInfo(Function.FunctionType.FuncLocalName, 0, 1, temparray2)
			},
			{
				"count",
				new ParamInfo(Function.FunctionType.FuncCount, 1, 1, temparray2)
			},
			{
				"id",
				new ParamInfo(Function.FunctionType.FuncID, 1, 1, temparray3)
			},
			{
				"string",
				new ParamInfo(Function.FunctionType.FuncString, 0, 1, temparray3)
			},
			{
				"concat",
				new ParamInfo(Function.FunctionType.FuncConcat, 2, 100, temparray4)
			},
			{
				"starts-with",
				new ParamInfo(Function.FunctionType.FuncStartsWith, 2, 2, temparray5)
			},
			{
				"contains",
				new ParamInfo(Function.FunctionType.FuncContains, 2, 2, temparray5)
			},
			{
				"substring-before",
				new ParamInfo(Function.FunctionType.FuncSubstringBefore, 2, 2, temparray5)
			},
			{
				"substring-after",
				new ParamInfo(Function.FunctionType.FuncSubstringAfter, 2, 2, temparray5)
			},
			{
				"substring",
				new ParamInfo(Function.FunctionType.FuncSubstring, 2, 3, temparray6)
			},
			{
				"string-length",
				new ParamInfo(Function.FunctionType.FuncStringLength, 0, 1, temparray4)
			},
			{
				"normalize-space",
				new ParamInfo(Function.FunctionType.FuncNormalize, 0, 1, temparray4)
			},
			{
				"translate",
				new ParamInfo(Function.FunctionType.FuncTranslate, 3, 3, temparray7)
			},
			{
				"boolean",
				new ParamInfo(Function.FunctionType.FuncBoolean, 1, 1, temparray3)
			},
			{
				"not",
				new ParamInfo(Function.FunctionType.FuncNot, 1, 1, temparray8)
			},
			{
				"true",
				new ParamInfo(Function.FunctionType.FuncTrue, 0, 0, temparray8)
			},
			{
				"false",
				new ParamInfo(Function.FunctionType.FuncFalse, 0, 0, temparray8)
			},
			{
				"lang",
				new ParamInfo(Function.FunctionType.FuncLang, 1, 1, temparray4)
			},
			{
				"number",
				new ParamInfo(Function.FunctionType.FuncNumber, 0, 1, temparray3)
			},
			{
				"sum",
				new ParamInfo(Function.FunctionType.FuncSum, 1, 1, temparray2)
			},
			{
				"floor",
				new ParamInfo(Function.FunctionType.FuncFloor, 1, 1, temparray9)
			},
			{
				"ceiling",
				new ParamInfo(Function.FunctionType.FuncCeiling, 1, 1, temparray9)
			},
			{
				"round",
				new ParamInfo(Function.FunctionType.FuncRound, 1, 1, temparray9)
			}
		};
	}

	private static Hashtable CreateAxesTable()
	{
		return new Hashtable(13)
		{
			{
				"ancestor",
				Axis.AxisType.Ancestor
			},
			{
				"ancestor-or-self",
				Axis.AxisType.AncestorOrSelf
			},
			{
				"attribute",
				Axis.AxisType.Attribute
			},
			{
				"child",
				Axis.AxisType.Child
			},
			{
				"descendant",
				Axis.AxisType.Descendant
			},
			{
				"descendant-or-self",
				Axis.AxisType.DescendantOrSelf
			},
			{
				"following",
				Axis.AxisType.Following
			},
			{
				"following-sibling",
				Axis.AxisType.FollowingSibling
			},
			{
				"namespace",
				Axis.AxisType.Namespace
			},
			{
				"parent",
				Axis.AxisType.Parent
			},
			{
				"preceding",
				Axis.AxisType.Preceding
			},
			{
				"preceding-sibling",
				Axis.AxisType.PrecedingSibling
			},
			{
				"self",
				Axis.AxisType.Self
			}
		};
	}

	private Axis.AxisType GetAxis(XPathScanner scaner)
	{
		return (Axis.AxisType)(AxesTable[scaner.Name] ?? throw XPathException.Create("'{0}' has an invalid token.", scanner.SourceText));
	}
}
