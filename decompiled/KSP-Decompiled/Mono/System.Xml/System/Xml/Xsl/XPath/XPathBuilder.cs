using System.Collections.Generic;
using System.Globalization;
using System.Xml.Schema;
using System.Xml.XPath;
using System.Xml.Xsl.Qil;

namespace System.Xml.Xsl.XPath;

internal class XPathBuilder : IXPathBuilder<QilNode>, IXPathEnvironment, IFocus
{
	private enum XPathOperatorGroup
	{
		Unknown,
		Logical,
		Equality,
		Relational,
		Arithmetic,
		Negate,
		Union
	}

	internal enum FuncId
	{
		Last,
		Position,
		Count,
		LocalName,
		NamespaceUri,
		Name,
		String,
		Number,
		Boolean,
		True,
		False,
		Not,
		Id,
		Concat,
		StartsWith,
		Contains,
		SubstringBefore,
		SubstringAfter,
		Substring,
		StringLength,
		Normalize,
		Translate,
		Lang,
		Sum,
		Floor,
		Ceiling,
		Round
	}

	internal class FixupVisitor : QilReplaceVisitor
	{
		private new QilPatternFactory f;

		private QilNode fixupCurrent;

		private QilNode fixupPosition;

		private QilNode fixupLast;

		private QilIterator current;

		private QilNode last;

		private bool justCount;

		private IXPathEnvironment environment;

		public int numCurrent;

		public int numPosition;

		public int numLast;

		public FixupVisitor(QilPatternFactory f, QilNode fixupCurrent, QilNode fixupPosition, QilNode fixupLast)
			: base(f.BaseFactory)
		{
			this.f = f;
			this.fixupCurrent = fixupCurrent;
			this.fixupPosition = fixupPosition;
			this.fixupLast = fixupLast;
		}

		public QilNode Fixup(QilNode inExpr, QilIterator current, QilNode last)
		{
			QilDepthChecker.Check(inExpr);
			this.current = current;
			this.last = last;
			justCount = false;
			environment = null;
			numCurrent = (numPosition = (numLast = 0));
			inExpr = VisitAssumeReference(inExpr);
			return inExpr;
		}

		public QilNode Fixup(QilNode inExpr, IXPathEnvironment environment)
		{
			QilDepthChecker.Check(inExpr);
			justCount = false;
			current = null;
			this.environment = environment;
			numCurrent = (numPosition = (numLast = 0));
			inExpr = VisitAssumeReference(inExpr);
			return inExpr;
		}

		public int CountUnfixedLast(QilNode inExpr)
		{
			justCount = true;
			numCurrent = (numPosition = (numLast = 0));
			VisitAssumeReference(inExpr);
			return numLast;
		}

		protected override QilNode VisitUnknown(QilNode unknown)
		{
			if (unknown == fixupCurrent)
			{
				numCurrent++;
				if (!justCount)
				{
					if (environment != null)
					{
						unknown = environment.GetCurrent();
					}
					else if (current != null)
					{
						unknown = current;
					}
				}
			}
			else if (unknown == fixupPosition)
			{
				numPosition++;
				if (!justCount)
				{
					if (environment != null)
					{
						unknown = environment.GetPosition();
					}
					else if (current != null)
					{
						unknown = f.XsltConvert(f.PositionOf(current), XmlQueryTypeFactory.DoubleX);
					}
				}
			}
			else if (unknown == fixupLast)
			{
				numLast++;
				if (!justCount)
				{
					if (environment != null)
					{
						unknown = environment.GetLast();
					}
					else if (current != null)
					{
						unknown = last;
					}
				}
			}
			return unknown;
		}
	}

	internal class FunctionInfo<T>
	{
		public T id;

		public int minArgs;

		public int maxArgs;

		public XmlTypeCode[] argTypes;

		public const int Infinity = int.MaxValue;

		public FunctionInfo(T id, int minArgs, int maxArgs, XmlTypeCode[] argTypes)
		{
			this.id = id;
			this.minArgs = minArgs;
			this.maxArgs = maxArgs;
			this.argTypes = argTypes;
		}

		public static void CheckArity(int minArgs, int maxArgs, string name, int numArgs)
		{
			if (minArgs <= numArgs && numArgs <= maxArgs)
			{
				return;
			}
			string resId = ((minArgs == maxArgs) ? "Function '{0}()' must have {1} argument(s)." : ((maxArgs == minArgs + 1) ? "Function '{0}()' must have {1} or {2} argument(s)." : ((numArgs >= minArgs) ? "Function '{0}()' must have no more than {2} arguments." : "Function '{0}()' must have at least {1} argument(s).")));
			throw new XPathCompileException(resId, name, minArgs.ToString(CultureInfo.InvariantCulture), maxArgs.ToString(CultureInfo.InvariantCulture));
		}

		public void CastArguments(IList<QilNode> args, string name, XPathQilFactory f)
		{
			CheckArity(minArgs, maxArgs, name, args.Count);
			if (maxArgs == int.MaxValue)
			{
				for (int i = 0; i < args.Count; i++)
				{
					args[i] = f.ConvertToType(XmlTypeCode.String, args[i]);
				}
				return;
			}
			for (int j = 0; j < args.Count; j++)
			{
				if (argTypes[j] == XmlTypeCode.Node && f.CannotBeNodeSet(args[j]))
				{
					throw new XPathCompileException("Argument {1} of function '{0}()' cannot be converted to a node-set.", name, (j + 1).ToString(CultureInfo.InvariantCulture));
				}
				args[j] = f.ConvertToType(argTypes[j], args[j]);
			}
		}
	}

	private XPathQilFactory f;

	private IXPathEnvironment environment;

	private bool inTheBuild;

	protected QilNode fixupCurrent;

	protected QilNode fixupPosition;

	protected QilNode fixupLast;

	protected int numFixupCurrent;

	protected int numFixupPosition;

	protected int numFixupLast;

	private FixupVisitor fixupVisitor;

	private static XmlNodeKindFlags[] XPathNodeType2QilXmlNodeKind = new XmlNodeKindFlags[10]
	{
		XmlNodeKindFlags.Document,
		XmlNodeKindFlags.Element,
		XmlNodeKindFlags.Attribute,
		XmlNodeKindFlags.Namespace,
		XmlNodeKindFlags.Text,
		XmlNodeKindFlags.Text,
		XmlNodeKindFlags.Text,
		XmlNodeKindFlags.PI,
		XmlNodeKindFlags.Comment,
		XmlNodeKindFlags.Any
	};

	private static XPathOperatorGroup[] OperatorGroup = new XPathOperatorGroup[16]
	{
		XPathOperatorGroup.Unknown,
		XPathOperatorGroup.Logical,
		XPathOperatorGroup.Logical,
		XPathOperatorGroup.Equality,
		XPathOperatorGroup.Equality,
		XPathOperatorGroup.Relational,
		XPathOperatorGroup.Relational,
		XPathOperatorGroup.Relational,
		XPathOperatorGroup.Relational,
		XPathOperatorGroup.Arithmetic,
		XPathOperatorGroup.Arithmetic,
		XPathOperatorGroup.Arithmetic,
		XPathOperatorGroup.Arithmetic,
		XPathOperatorGroup.Arithmetic,
		XPathOperatorGroup.Negate,
		XPathOperatorGroup.Union
	};

	private static QilNodeType[] QilOperator = new QilNodeType[16]
	{
		QilNodeType.Unknown,
		QilNodeType.Or,
		QilNodeType.And,
		QilNodeType.Eq,
		QilNodeType.Ne,
		QilNodeType.Lt,
		QilNodeType.Le,
		QilNodeType.Gt,
		QilNodeType.Ge,
		QilNodeType.Add,
		QilNodeType.Subtract,
		QilNodeType.Multiply,
		QilNodeType.Divide,
		QilNodeType.Modulo,
		QilNodeType.Negate,
		QilNodeType.Sequence
	};

	private static XmlNodeKindFlags[] XPathAxisMask = new XmlNodeKindFlags[15]
	{
		XmlNodeKindFlags.None,
		XmlNodeKindFlags.Document | XmlNodeKindFlags.Element,
		XmlNodeKindFlags.Any,
		XmlNodeKindFlags.Attribute,
		XmlNodeKindFlags.Content,
		XmlNodeKindFlags.Content,
		XmlNodeKindFlags.Any,
		XmlNodeKindFlags.Content,
		XmlNodeKindFlags.Content,
		XmlNodeKindFlags.Namespace,
		XmlNodeKindFlags.Document | XmlNodeKindFlags.Element,
		XmlNodeKindFlags.Content,
		XmlNodeKindFlags.Content,
		XmlNodeKindFlags.Any,
		XmlNodeKindFlags.Document
	};

	public static readonly XmlTypeCode[] argAny = new XmlTypeCode[1] { XmlTypeCode.Item };

	public static readonly XmlTypeCode[] argNodeSet = new XmlTypeCode[1] { XmlTypeCode.Node };

	public static readonly XmlTypeCode[] argBoolean = new XmlTypeCode[1] { XmlTypeCode.Boolean };

	public static readonly XmlTypeCode[] argDouble = new XmlTypeCode[1] { XmlTypeCode.Double };

	public static readonly XmlTypeCode[] argString = new XmlTypeCode[1] { XmlTypeCode.String };

	public static readonly XmlTypeCode[] argString2 = new XmlTypeCode[2]
	{
		XmlTypeCode.String,
		XmlTypeCode.String
	};

	public static readonly XmlTypeCode[] argString3 = new XmlTypeCode[3]
	{
		XmlTypeCode.String,
		XmlTypeCode.String,
		XmlTypeCode.String
	};

	public static readonly XmlTypeCode[] argFnSubstr = new XmlTypeCode[3]
	{
		XmlTypeCode.String,
		XmlTypeCode.Double,
		XmlTypeCode.Double
	};

	public static Dictionary<string, FunctionInfo<FuncId>> FunctionTable = CreateFunctionTable();

	XPathQilFactory IXPathEnvironment.Factory => f;

	QilNode IFocus.GetCurrent()
	{
		return GetCurrentNode();
	}

	QilNode IFocus.GetPosition()
	{
		return GetCurrentPosition();
	}

	QilNode IFocus.GetLast()
	{
		return GetLastPosition();
	}

	QilNode IXPathEnvironment.ResolveVariable(string prefix, string name)
	{
		return Variable(prefix, name);
	}

	QilNode IXPathEnvironment.ResolveFunction(string prefix, string name, IList<QilNode> args, IFocus env)
	{
		return null;
	}

	string IXPathEnvironment.ResolvePrefix(string prefix)
	{
		return environment.ResolvePrefix(prefix);
	}

	public XPathBuilder(IXPathEnvironment environment)
	{
		this.environment = environment;
		f = this.environment.Factory;
		fixupCurrent = f.Unknown(XmlQueryTypeFactory.NodeNotRtf);
		fixupPosition = f.Unknown(XmlQueryTypeFactory.DoubleX);
		fixupLast = f.Unknown(XmlQueryTypeFactory.DoubleX);
		fixupVisitor = new FixupVisitor(f, fixupCurrent, fixupPosition, fixupLast);
	}

	public virtual void StartBuild()
	{
		inTheBuild = true;
		numFixupCurrent = (numFixupPosition = (numFixupLast = 0));
	}

	public virtual QilNode EndBuild(QilNode result)
	{
		if (result == null)
		{
			inTheBuild = false;
			return result;
		}
		if (result.XmlType.MaybeMany && result.XmlType.IsNode && result.XmlType.IsNotRtf)
		{
			result = f.DocOrderDistinct(result);
		}
		result = fixupVisitor.Fixup(result, environment);
		numFixupCurrent -= fixupVisitor.numCurrent;
		numFixupPosition -= fixupVisitor.numPosition;
		numFixupLast -= fixupVisitor.numLast;
		inTheBuild = false;
		return result;
	}

	private QilNode GetCurrentNode()
	{
		numFixupCurrent++;
		return fixupCurrent;
	}

	private QilNode GetCurrentPosition()
	{
		numFixupPosition++;
		return fixupPosition;
	}

	private QilNode GetLastPosition()
	{
		numFixupLast++;
		return fixupLast;
	}

	public virtual QilNode String(string value)
	{
		return f.String(value);
	}

	public virtual QilNode Number(double value)
	{
		return f.Double(value);
	}

	public virtual QilNode Operator(XPathOperator op, QilNode left, QilNode right)
	{
		return OperatorGroup[(int)op] switch
		{
			XPathOperatorGroup.Logical => LogicalOperator(op, left, right), 
			XPathOperatorGroup.Equality => EqualityOperator(op, left, right), 
			XPathOperatorGroup.Relational => RelationalOperator(op, left, right), 
			XPathOperatorGroup.Arithmetic => ArithmeticOperator(op, left, right), 
			XPathOperatorGroup.Negate => NegateOperator(op, left, right), 
			XPathOperatorGroup.Union => UnionOperator(op, left, right), 
			_ => null, 
		};
	}

	private QilNode LogicalOperator(XPathOperator op, QilNode left, QilNode right)
	{
		left = f.ConvertToBoolean(left);
		right = f.ConvertToBoolean(right);
		if (op != XPathOperator.Or)
		{
			return f.And(left, right);
		}
		return f.Or(left, right);
	}

	private QilNode CompareValues(XPathOperator op, QilNode left, QilNode right, XmlTypeCode compType)
	{
		left = f.ConvertToType(compType, left);
		right = f.ConvertToType(compType, right);
		return op switch
		{
			XPathOperator.Eq => f.Eq(left, right), 
			XPathOperator.Ne => f.Ne(left, right), 
			XPathOperator.Lt => f.Lt(left, right), 
			XPathOperator.Le => f.Le(left, right), 
			XPathOperator.Gt => f.Gt(left, right), 
			XPathOperator.Ge => f.Ge(left, right), 
			_ => null, 
		};
	}

	private QilNode CompareNodeSetAndValue(XPathOperator op, QilNode nodeset, QilNode val, XmlTypeCode compType)
	{
		if (compType == XmlTypeCode.Boolean || nodeset.XmlType.IsSingleton)
		{
			return CompareValues(op, nodeset, val, compType);
		}
		QilIterator qilIterator = f.For(nodeset);
		return f.Not(f.IsEmpty(f.Filter(qilIterator, CompareValues(op, f.XPathNodeValue(qilIterator), val, compType))));
	}

	private static XPathOperator InvertOp(XPathOperator op)
	{
		return op switch
		{
			XPathOperator.Ge => XPathOperator.Le, 
			XPathOperator.Gt => XPathOperator.Lt, 
			XPathOperator.Le => XPathOperator.Ge, 
			XPathOperator.Lt => XPathOperator.Gt, 
			_ => op, 
		};
	}

	private QilNode CompareNodeSetAndNodeSet(XPathOperator op, QilNode left, QilNode right, XmlTypeCode compType)
	{
		if (right.XmlType.IsSingleton)
		{
			return CompareNodeSetAndValue(op, left, right, compType);
		}
		if (left.XmlType.IsSingleton)
		{
			op = InvertOp(op);
			return CompareNodeSetAndValue(op, right, left, compType);
		}
		QilIterator qilIterator = f.For(left);
		QilIterator qilIterator2 = f.For(right);
		return f.Not(f.IsEmpty(f.Loop(qilIterator, f.Filter(qilIterator2, CompareValues(op, f.XPathNodeValue(qilIterator), f.XPathNodeValue(qilIterator2), compType)))));
	}

	private QilNode EqualityOperator(XPathOperator op, QilNode left, QilNode right)
	{
		XmlQueryType xmlType = left.XmlType;
		XmlQueryType xmlType2 = right.XmlType;
		if (f.IsAnyType(left) || f.IsAnyType(right))
		{
			return f.InvokeEqualityOperator(QilOperator[(int)op], left, right);
		}
		if (xmlType.IsNode && xmlType2.IsNode)
		{
			return CompareNodeSetAndNodeSet(op, left, right, XmlTypeCode.String);
		}
		if (xmlType.IsNode)
		{
			return CompareNodeSetAndValue(op, left, right, xmlType2.TypeCode);
		}
		if (xmlType2.IsNode)
		{
			return CompareNodeSetAndValue(op, right, left, xmlType.TypeCode);
		}
		XmlTypeCode compType = ((xmlType.TypeCode == XmlTypeCode.Boolean || xmlType2.TypeCode == XmlTypeCode.Boolean) ? XmlTypeCode.Boolean : ((xmlType.TypeCode == XmlTypeCode.Double || xmlType2.TypeCode == XmlTypeCode.Double) ? XmlTypeCode.Double : XmlTypeCode.String));
		return CompareValues(op, left, right, compType);
	}

	private QilNode RelationalOperator(XPathOperator op, QilNode left, QilNode right)
	{
		XmlQueryType xmlType = left.XmlType;
		XmlQueryType xmlType2 = right.XmlType;
		if (f.IsAnyType(left) || f.IsAnyType(right))
		{
			return f.InvokeRelationalOperator(QilOperator[(int)op], left, right);
		}
		if (xmlType.IsNode && xmlType2.IsNode)
		{
			return CompareNodeSetAndNodeSet(op, left, right, XmlTypeCode.Double);
		}
		if (xmlType.IsNode)
		{
			XmlTypeCode compType = ((xmlType2.TypeCode == XmlTypeCode.Boolean) ? XmlTypeCode.Boolean : XmlTypeCode.Double);
			return CompareNodeSetAndValue(op, left, right, compType);
		}
		if (xmlType2.IsNode)
		{
			XmlTypeCode compType2 = ((xmlType.TypeCode == XmlTypeCode.Boolean) ? XmlTypeCode.Boolean : XmlTypeCode.Double);
			op = InvertOp(op);
			return CompareNodeSetAndValue(op, right, left, compType2);
		}
		return CompareValues(op, left, right, XmlTypeCode.Double);
	}

	private QilNode NegateOperator(XPathOperator op, QilNode left, QilNode right)
	{
		return f.Negate(f.ConvertToNumber(left));
	}

	private QilNode ArithmeticOperator(XPathOperator op, QilNode left, QilNode right)
	{
		left = f.ConvertToNumber(left);
		right = f.ConvertToNumber(right);
		return op switch
		{
			XPathOperator.Plus => f.Add(left, right), 
			XPathOperator.Minus => f.Subtract(left, right), 
			XPathOperator.Multiply => f.Multiply(left, right), 
			XPathOperator.Divide => f.Divide(left, right), 
			XPathOperator.Modulo => f.Modulo(left, right), 
			_ => null, 
		};
	}

	private QilNode UnionOperator(XPathOperator op, QilNode left, QilNode right)
	{
		if (left == null)
		{
			return f.EnsureNodeSet(right);
		}
		left = f.EnsureNodeSet(left);
		right = f.EnsureNodeSet(right);
		if (left.NodeType == QilNodeType.Sequence)
		{
			((QilList)left).Add(right);
			return left;
		}
		return f.Union(left, right);
	}

	public static XmlNodeKindFlags AxisTypeMask(XmlNodeKindFlags inputTypeMask, XPathNodeType nodeType, XPathAxis xpathAxis)
	{
		return inputTypeMask & XPathNodeType2QilXmlNodeKind[(int)nodeType] & XPathAxisMask[(int)xpathAxis];
	}

	private QilNode BuildAxisFilter(QilNode qilAxis, XPathAxis xpathAxis, XPathNodeType nodeType, string name, string nsUri)
	{
		XmlNodeKindFlags nodeKinds = qilAxis.XmlType.NodeKinds;
		XmlNodeKindFlags xmlNodeKindFlags = AxisTypeMask(nodeKinds, nodeType, xpathAxis);
		if (xmlNodeKindFlags == XmlNodeKindFlags.None)
		{
			return f.Sequence();
		}
		QilIterator expr;
		if (xmlNodeKindFlags != nodeKinds)
		{
			qilAxis = f.Filter(expr = f.For(qilAxis), f.IsType(expr, XmlQueryTypeFactory.NodeChoice(xmlNodeKindFlags)));
			qilAxis.XmlType = XmlQueryTypeFactory.PrimeProduct(XmlQueryTypeFactory.NodeChoice(xmlNodeKindFlags), qilAxis.XmlType.Cardinality);
			if (qilAxis.NodeType == QilNodeType.Filter)
			{
				QilLoop qilLoop = (QilLoop)qilAxis;
				qilLoop.Body = f.And(qilLoop.Body, (name != null && nsUri != null) ? f.Eq(f.NameOf(expr), f.QName(name, nsUri)) : ((nsUri != null) ? f.Eq(f.NamespaceUriOf(expr), f.String(nsUri)) : ((name != null) ? f.Eq(f.LocalNameOf(expr), f.String(name)) : f.True())));
				return qilLoop;
			}
		}
		return f.Filter(expr = f.For(qilAxis), (name != null && nsUri != null) ? f.Eq(f.NameOf(expr), f.QName(name, nsUri)) : ((nsUri != null) ? f.Eq(f.NamespaceUriOf(expr), f.String(nsUri)) : ((name != null) ? f.Eq(f.LocalNameOf(expr), f.String(name)) : f.True())));
	}

	private QilNode BuildAxis(XPathAxis xpathAxis, XPathNodeType nodeType, string nsUri, string name)
	{
		QilNode currentNode = GetCurrentNode();
		QilNode qilAxis;
		switch (xpathAxis)
		{
		case XPathAxis.Ancestor:
			qilAxis = f.Ancestor(currentNode);
			break;
		case XPathAxis.AncestorOrSelf:
			qilAxis = f.AncestorOrSelf(currentNode);
			break;
		case XPathAxis.Attribute:
			qilAxis = f.Content(currentNode);
			break;
		case XPathAxis.Child:
			qilAxis = f.Content(currentNode);
			break;
		case XPathAxis.Descendant:
			qilAxis = f.Descendant(currentNode);
			break;
		case XPathAxis.DescendantOrSelf:
			qilAxis = f.DescendantOrSelf(currentNode);
			break;
		case XPathAxis.Following:
			qilAxis = f.XPathFollowing(currentNode);
			break;
		case XPathAxis.FollowingSibling:
			qilAxis = f.FollowingSibling(currentNode);
			break;
		case XPathAxis.Namespace:
			qilAxis = f.XPathNamespace(currentNode);
			break;
		case XPathAxis.Parent:
			qilAxis = f.Parent(currentNode);
			break;
		case XPathAxis.Preceding:
			qilAxis = f.XPathPreceding(currentNode);
			break;
		case XPathAxis.PrecedingSibling:
			qilAxis = f.PrecedingSibling(currentNode);
			break;
		case XPathAxis.Self:
			qilAxis = currentNode;
			break;
		case XPathAxis.Root:
			return f.Root(currentNode);
		default:
			qilAxis = null;
			break;
		}
		QilNode qilNode = BuildAxisFilter(qilAxis, xpathAxis, nodeType, name, nsUri);
		if (xpathAxis == XPathAxis.Ancestor || xpathAxis == XPathAxis.Preceding || xpathAxis == XPathAxis.AncestorOrSelf || xpathAxis == XPathAxis.PrecedingSibling)
		{
			qilNode = f.BaseFactory.DocOrderDistinct(qilNode);
		}
		return qilNode;
	}

	public virtual QilNode Axis(XPathAxis xpathAxis, XPathNodeType nodeType, string prefix, string name)
	{
		string nsUri = ((prefix == null) ? null : environment.ResolvePrefix(prefix));
		return BuildAxis(xpathAxis, nodeType, nsUri, name);
	}

	public virtual QilNode JoinStep(QilNode left, QilNode right)
	{
		QilIterator qilIterator = f.For(f.EnsureNodeSet(left));
		right = fixupVisitor.Fixup(right, qilIterator, null);
		numFixupCurrent -= fixupVisitor.numCurrent;
		numFixupPosition -= fixupVisitor.numPosition;
		numFixupLast -= fixupVisitor.numLast;
		return f.DocOrderDistinct(f.Loop(qilIterator, right));
	}

	public virtual QilNode Predicate(QilNode nodeset, QilNode predicate, bool isReverseStep)
	{
		if (isReverseStep)
		{
			nodeset = ((QilUnary)nodeset).Child;
		}
		predicate = PredicateToBoolean(predicate, f, this);
		return BuildOnePredicate(nodeset, predicate, isReverseStep, f, fixupVisitor, ref numFixupCurrent, ref numFixupPosition, ref numFixupLast);
	}

	public static QilNode PredicateToBoolean(QilNode predicate, XPathQilFactory f, IXPathEnvironment env)
	{
		QilIterator qilIterator;
		predicate = (f.IsAnyType(predicate) ? f.Loop(qilIterator = f.Let(predicate), f.Conditional(f.IsType(qilIterator, XmlQueryTypeFactory.Double), f.Eq(env.GetPosition(), f.TypeAssert(qilIterator, XmlQueryTypeFactory.DoubleX)), f.ConvertToBoolean(qilIterator))) : ((predicate.XmlType.TypeCode != XmlTypeCode.Double) ? f.ConvertToBoolean(predicate) : f.Eq(env.GetPosition(), predicate)));
		return predicate;
	}

	public static QilNode BuildOnePredicate(QilNode nodeset, QilNode predicate, bool isReverseStep, XPathQilFactory f, FixupVisitor fixupVisitor, ref int numFixupCurrent, ref int numFixupPosition, ref int numFixupLast)
	{
		nodeset = f.EnsureNodeSet(nodeset);
		QilNode qilNode;
		if (numFixupLast != 0 && fixupVisitor.CountUnfixedLast(predicate) != 0)
		{
			QilIterator qilIterator = f.Let(nodeset);
			QilIterator qilIterator2 = f.Let(f.XsltConvert(f.Length(qilIterator), XmlQueryTypeFactory.DoubleX));
			QilIterator qilIterator3 = f.For(qilIterator);
			predicate = fixupVisitor.Fixup(predicate, qilIterator3, qilIterator2);
			numFixupCurrent -= fixupVisitor.numCurrent;
			numFixupPosition -= fixupVisitor.numPosition;
			numFixupLast -= fixupVisitor.numLast;
			qilNode = f.Loop(qilIterator, f.Loop(qilIterator2, f.Filter(qilIterator3, predicate)));
		}
		else
		{
			QilIterator qilIterator4 = f.For(nodeset);
			predicate = fixupVisitor.Fixup(predicate, qilIterator4, null);
			numFixupCurrent -= fixupVisitor.numCurrent;
			numFixupPosition -= fixupVisitor.numPosition;
			numFixupLast -= fixupVisitor.numLast;
			qilNode = f.Filter(qilIterator4, predicate);
		}
		if (isReverseStep)
		{
			qilNode = f.DocOrderDistinct(qilNode);
		}
		return qilNode;
	}

	public virtual QilNode Variable(string prefix, string name)
	{
		return environment.ResolveVariable(prefix, name);
	}

	public virtual QilNode Function(string prefix, string name, IList<QilNode> args)
	{
		if (prefix.Length == 0 && FunctionTable.TryGetValue(name, out var value))
		{
			value.CastArguments(args, name, f);
			switch (value.id)
			{
			case FuncId.Not:
				return f.Not(args[0]);
			case FuncId.Last:
				return GetLastPosition();
			case FuncId.Position:
				return GetCurrentPosition();
			case FuncId.Count:
				return f.XsltConvert(f.Length(f.DocOrderDistinct(args[0])), XmlQueryTypeFactory.DoubleX);
			case FuncId.LocalName:
				if (args.Count != 0)
				{
					return LocalNameOfFirstNode(args[0]);
				}
				return f.LocalNameOf(GetCurrentNode());
			case FuncId.NamespaceUri:
				if (args.Count != 0)
				{
					return NamespaceOfFirstNode(args[0]);
				}
				return f.NamespaceUriOf(GetCurrentNode());
			case FuncId.Name:
				if (args.Count != 0)
				{
					return NameOfFirstNode(args[0]);
				}
				return NameOf(GetCurrentNode());
			case FuncId.String:
				if (args.Count != 0)
				{
					return f.ConvertToString(args[0]);
				}
				return f.XPathNodeValue(GetCurrentNode());
			case FuncId.Number:
				if (args.Count != 0)
				{
					return f.ConvertToNumber(args[0]);
				}
				return f.XsltConvert(f.XPathNodeValue(GetCurrentNode()), XmlQueryTypeFactory.DoubleX);
			case FuncId.Boolean:
				return f.ConvertToBoolean(args[0]);
			case FuncId.True:
				return f.True();
			case FuncId.False:
				return f.False();
			case FuncId.Id:
				return f.DocOrderDistinct(f.Id(GetCurrentNode(), args[0]));
			case FuncId.Concat:
				return f.StrConcat(args);
			case FuncId.StartsWith:
				return f.InvokeStartsWith(args[0], args[1]);
			case FuncId.Contains:
				return f.InvokeContains(args[0], args[1]);
			case FuncId.SubstringBefore:
				return f.InvokeSubstringBefore(args[0], args[1]);
			case FuncId.SubstringAfter:
				return f.InvokeSubstringAfter(args[0], args[1]);
			case FuncId.Substring:
				if (args.Count != 2)
				{
					return f.InvokeSubstring(args[0], args[1], args[2]);
				}
				return f.InvokeSubstring(args[0], args[1]);
			case FuncId.StringLength:
				return f.XsltConvert(f.StrLength((args.Count == 0) ? f.XPathNodeValue(GetCurrentNode()) : args[0]), XmlQueryTypeFactory.DoubleX);
			case FuncId.Normalize:
				return f.InvokeNormalizeSpace((args.Count == 0) ? f.XPathNodeValue(GetCurrentNode()) : args[0]);
			case FuncId.Translate:
				return f.InvokeTranslate(args[0], args[1], args[2]);
			case FuncId.Lang:
				return f.InvokeLang(args[0], GetCurrentNode());
			case FuncId.Sum:
				return Sum(f.DocOrderDistinct(args[0]));
			case FuncId.Floor:
				return f.InvokeFloor(args[0]);
			case FuncId.Ceiling:
				return f.InvokeCeiling(args[0]);
			case FuncId.Round:
				return f.InvokeRound(args[0]);
			default:
				return null;
			}
		}
		return environment.ResolveFunction(prefix, name, args, this);
	}

	private QilNode LocalNameOfFirstNode(QilNode arg)
	{
		if (arg.XmlType.IsSingleton)
		{
			return f.LocalNameOf(arg);
		}
		QilIterator expr;
		return f.StrConcat(f.Loop(expr = f.FirstNode(arg), f.LocalNameOf(expr)));
	}

	private QilNode NamespaceOfFirstNode(QilNode arg)
	{
		if (arg.XmlType.IsSingleton)
		{
			return f.NamespaceUriOf(arg);
		}
		QilIterator expr;
		return f.StrConcat(f.Loop(expr = f.FirstNode(arg), f.NamespaceUriOf(expr)));
	}

	private QilNode NameOf(QilNode arg)
	{
		QilIterator qilIterator;
		QilIterator qilIterator2;
		if (arg is QilIterator)
		{
			return f.Loop(qilIterator = f.Let(f.PrefixOf(arg)), f.Loop(qilIterator2 = f.Let(f.LocalNameOf(arg)), f.Conditional(f.Eq(f.StrLength(qilIterator), f.Int32(0)), qilIterator2, f.StrConcat(qilIterator, f.String(":"), qilIterator2))));
		}
		QilIterator qilIterator3 = f.Let(arg);
		return f.Loop(qilIterator3, NameOf(qilIterator3));
	}

	private QilNode NameOfFirstNode(QilNode arg)
	{
		if (arg.XmlType.IsSingleton)
		{
			return NameOf(arg);
		}
		QilIterator arg2;
		return f.StrConcat(f.Loop(arg2 = f.FirstNode(arg), NameOf(arg2)));
	}

	private QilNode Sum(QilNode arg)
	{
		QilIterator n;
		return f.Sum(f.Sequence(f.Double(0.0), f.Loop(n = f.For(arg), f.ConvertToNumber(n))));
	}

	private static Dictionary<string, FunctionInfo<FuncId>> CreateFunctionTable()
	{
		return new Dictionary<string, FunctionInfo<FuncId>>(36)
		{
			{
				"last",
				new FunctionInfo<FuncId>(FuncId.Last, 0, 0, null)
			},
			{
				"position",
				new FunctionInfo<FuncId>(FuncId.Position, 0, 0, null)
			},
			{
				"name",
				new FunctionInfo<FuncId>(FuncId.Name, 0, 1, argNodeSet)
			},
			{
				"namespace-uri",
				new FunctionInfo<FuncId>(FuncId.NamespaceUri, 0, 1, argNodeSet)
			},
			{
				"local-name",
				new FunctionInfo<FuncId>(FuncId.LocalName, 0, 1, argNodeSet)
			},
			{
				"count",
				new FunctionInfo<FuncId>(FuncId.Count, 1, 1, argNodeSet)
			},
			{
				"id",
				new FunctionInfo<FuncId>(FuncId.Id, 1, 1, argAny)
			},
			{
				"string",
				new FunctionInfo<FuncId>(FuncId.String, 0, 1, argAny)
			},
			{
				"concat",
				new FunctionInfo<FuncId>(FuncId.Concat, 2, int.MaxValue, null)
			},
			{
				"starts-with",
				new FunctionInfo<FuncId>(FuncId.StartsWith, 2, 2, argString2)
			},
			{
				"contains",
				new FunctionInfo<FuncId>(FuncId.Contains, 2, 2, argString2)
			},
			{
				"substring-before",
				new FunctionInfo<FuncId>(FuncId.SubstringBefore, 2, 2, argString2)
			},
			{
				"substring-after",
				new FunctionInfo<FuncId>(FuncId.SubstringAfter, 2, 2, argString2)
			},
			{
				"substring",
				new FunctionInfo<FuncId>(FuncId.Substring, 2, 3, argFnSubstr)
			},
			{
				"string-length",
				new FunctionInfo<FuncId>(FuncId.StringLength, 0, 1, argString)
			},
			{
				"normalize-space",
				new FunctionInfo<FuncId>(FuncId.Normalize, 0, 1, argString)
			},
			{
				"translate",
				new FunctionInfo<FuncId>(FuncId.Translate, 3, 3, argString3)
			},
			{
				"boolean",
				new FunctionInfo<FuncId>(FuncId.Boolean, 1, 1, argAny)
			},
			{
				"not",
				new FunctionInfo<FuncId>(FuncId.Not, 1, 1, argBoolean)
			},
			{
				"true",
				new FunctionInfo<FuncId>(FuncId.True, 0, 0, null)
			},
			{
				"false",
				new FunctionInfo<FuncId>(FuncId.False, 0, 0, null)
			},
			{
				"lang",
				new FunctionInfo<FuncId>(FuncId.Lang, 1, 1, argString)
			},
			{
				"number",
				new FunctionInfo<FuncId>(FuncId.Number, 0, 1, argAny)
			},
			{
				"sum",
				new FunctionInfo<FuncId>(FuncId.Sum, 1, 1, argNodeSet)
			},
			{
				"floor",
				new FunctionInfo<FuncId>(FuncId.Floor, 1, 1, argDouble)
			},
			{
				"ceiling",
				new FunctionInfo<FuncId>(FuncId.Ceiling, 1, 1, argDouble)
			},
			{
				"round",
				new FunctionInfo<FuncId>(FuncId.Round, 1, 1, argDouble)
			}
		};
	}

	public static bool IsFunctionAvailable(string localName, string nsUri)
	{
		if (nsUri.Length != 0)
		{
			return false;
		}
		return FunctionTable.ContainsKey(localName);
	}
}
