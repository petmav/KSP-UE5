using System.Collections.Generic;
using System.Diagnostics;
using System.Globalization;
using System.Xml.XPath;
using System.Xml.Xsl.Qil;
using System.Xml.Xsl.XPath;

namespace System.Xml.Xsl.Xslt;

internal class XPathPatternBuilder : XPathPatternParser.IPatternBuilder, IXPathBuilder<QilNode>
{
	private class Annotation
	{
		public double Priority;

		public QilLoop Parent;
	}

	private class XPathPredicateEnvironment : IXPathEnvironment, IFocus
	{
		private readonly IXPathEnvironment baseEnvironment;

		private readonly XPathQilFactory f;

		public readonly XPathBuilder.FixupVisitor fixupVisitor;

		private readonly QilNode fixupCurrent;

		private readonly QilNode fixupPosition;

		private readonly QilNode fixupLast;

		public int numFixupCurrent;

		public int numFixupPosition;

		public int numFixupLast;

		public XPathQilFactory Factory => f;

		public XPathPredicateEnvironment(IXPathEnvironment baseEnvironment)
		{
			this.baseEnvironment = baseEnvironment;
			f = baseEnvironment.Factory;
			fixupCurrent = f.Unknown(XmlQueryTypeFactory.NodeNotRtf);
			fixupPosition = f.Unknown(XmlQueryTypeFactory.DoubleX);
			fixupLast = f.Unknown(XmlQueryTypeFactory.DoubleX);
			fixupVisitor = new XPathBuilder.FixupVisitor(f, fixupCurrent, fixupPosition, fixupLast);
		}

		public QilNode ResolveVariable(string prefix, string name)
		{
			return baseEnvironment.ResolveVariable(prefix, name);
		}

		public QilNode ResolveFunction(string prefix, string name, IList<QilNode> args, IFocus env)
		{
			return baseEnvironment.ResolveFunction(prefix, name, args, env);
		}

		public string ResolvePrefix(string prefix)
		{
			return baseEnvironment.ResolvePrefix(prefix);
		}

		public QilNode GetCurrent()
		{
			numFixupCurrent++;
			return fixupCurrent;
		}

		public QilNode GetPosition()
		{
			numFixupPosition++;
			return fixupPosition;
		}

		public QilNode GetLast()
		{
			numFixupLast++;
			return fixupLast;
		}
	}

	private class XsltFunctionFocus : IFocus
	{
		private QilIterator current;

		public XsltFunctionFocus(QilIterator current)
		{
			this.current = current;
		}

		public QilNode GetCurrent()
		{
			return current;
		}

		public QilNode GetPosition()
		{
			return null;
		}

		public QilNode GetLast()
		{
			return null;
		}
	}

	private XPathPredicateEnvironment predicateEnvironment;

	private XPathBuilder predicateBuilder;

	private bool inTheBuild;

	private XPathQilFactory f;

	private QilNode fixupNode;

	private IXPathEnvironment environment;

	public QilNode FixupNode => fixupNode;

	public XPathPatternBuilder(IXPathEnvironment environment)
	{
		this.environment = environment;
		f = environment.Factory;
		predicateEnvironment = new XPathPredicateEnvironment(environment);
		predicateBuilder = new XPathBuilder(predicateEnvironment);
		fixupNode = f.Unknown(XmlQueryTypeFactory.NodeNotRtfS);
	}

	public virtual void StartBuild()
	{
		inTheBuild = true;
	}

	[Conditional("DEBUG")]
	public void AssertFilter(QilLoop filter)
	{
	}

	private void FixupFilterBinding(QilLoop filter, QilNode newBinding)
	{
		filter.Variable.Binding = newBinding;
	}

	public virtual QilNode EndBuild(QilNode result)
	{
		inTheBuild = false;
		return result;
	}

	public QilNode Operator(XPathOperator op, QilNode left, QilNode right)
	{
		if (left.NodeType == QilNodeType.Sequence)
		{
			((QilList)left).Add(right);
			return left;
		}
		return f.Sequence(left, right);
	}

	private static QilLoop BuildAxisFilter(QilPatternFactory f, QilIterator itr, XPathAxis xpathAxis, XPathNodeType nodeType, string name, string nsUri)
	{
		QilNode right = ((name != null && nsUri != null) ? f.Eq(f.NameOf(itr), f.QName(name, nsUri)) : ((nsUri != null) ? f.Eq(f.NamespaceUriOf(itr), f.String(nsUri)) : ((name != null) ? f.Eq(f.LocalNameOf(itr), f.String(name)) : f.True())));
		XmlNodeKindFlags xmlNodeKindFlags = XPathBuilder.AxisTypeMask(itr.XmlType.NodeKinds, nodeType, xpathAxis);
		QilNode left = ((xmlNodeKindFlags == XmlNodeKindFlags.None) ? f.False() : ((xmlNodeKindFlags == itr.XmlType.NodeKinds) ? f.True() : f.IsType(itr, XmlQueryTypeFactory.NodeChoice(xmlNodeKindFlags))));
		QilLoop qilLoop = f.BaseFactory.Filter(itr, f.And(left, right));
		qilLoop.XmlType = XmlQueryTypeFactory.PrimeProduct(XmlQueryTypeFactory.NodeChoice(xmlNodeKindFlags), qilLoop.XmlType.Cardinality);
		return qilLoop;
	}

	public QilNode Axis(XPathAxis xpathAxis, XPathNodeType nodeType, string prefix, string name)
	{
		QilLoop qilLoop;
		double priority;
		switch (xpathAxis)
		{
		case XPathAxis.DescendantOrSelf:
			return f.Nop(fixupNode);
		case XPathAxis.Root:
		{
			QilIterator expr;
			qilLoop = f.BaseFactory.Filter(expr = f.For(fixupNode), f.IsType(expr, XmlQueryTypeFactory.Document));
			priority = 0.5;
			break;
		}
		default:
		{
			string nsUri = ((prefix == null) ? null : environment.ResolvePrefix(prefix));
			qilLoop = BuildAxisFilter(f, f.For(fixupNode), xpathAxis, nodeType, name, nsUri);
			switch (nodeType)
			{
			case XPathNodeType.Element:
			case XPathNodeType.Attribute:
				priority = ((name == null) ? ((prefix == null) ? (-0.5) : (-0.25)) : 0.0);
				break;
			case XPathNodeType.ProcessingInstruction:
				priority = ((name != null) ? 0.0 : (-0.5));
				break;
			default:
				priority = -0.5;
				break;
			}
			break;
		}
		}
		SetPriority(qilLoop, priority);
		SetLastParent(qilLoop, qilLoop);
		return qilLoop;
	}

	public QilNode JoinStep(QilNode left, QilNode right)
	{
		if (left.NodeType == QilNodeType.Nop)
		{
			QilUnary obj = (QilUnary)left;
			obj.Child = right;
			return obj;
		}
		CleanAnnotation(left);
		QilLoop qilLoop = (QilLoop)left;
		bool flag = false;
		if (right.NodeType == QilNodeType.Nop)
		{
			flag = true;
			right = ((QilUnary)right).Child;
		}
		QilLoop lastParent = GetLastParent(right);
		FixupFilterBinding(qilLoop, flag ? f.Ancestor(lastParent.Variable) : f.Parent(lastParent.Variable));
		lastParent.Body = f.And(lastParent.Body, f.Not(f.IsEmpty(qilLoop)));
		SetPriority(right, 0.5);
		SetLastParent(right, qilLoop);
		return right;
	}

	QilNode IXPathBuilder<QilNode>.Predicate(QilNode node, QilNode condition, bool isReverseStep)
	{
		return null;
	}

	public QilNode BuildPredicates(QilNode nodeset, List<QilNode> predicates)
	{
		List<QilNode> list = new List<QilNode>(predicates.Count);
		foreach (QilNode predicate in predicates)
		{
			list.Add(XPathBuilder.PredicateToBoolean(predicate, f, predicateEnvironment));
		}
		QilLoop qilLoop = (QilLoop)nodeset;
		QilIterator variable = qilLoop.Variable;
		if (predicateEnvironment.numFixupLast == 0 && predicateEnvironment.numFixupPosition == 0)
		{
			foreach (QilNode item in list)
			{
				qilLoop.Body = f.And(qilLoop.Body, item);
			}
			qilLoop.Body = predicateEnvironment.fixupVisitor.Fixup(qilLoop.Body, variable, null);
		}
		else
		{
			QilIterator qilIterator = f.For(f.Parent(variable));
			QilNode binding = f.Content(qilIterator);
			QilLoop qilLoop2 = (QilLoop)nodeset.DeepClone(f.BaseFactory);
			qilLoop2.Variable.Binding = binding;
			qilLoop2 = (QilLoop)f.Loop(qilIterator, qilLoop2);
			QilNode qilNode = qilLoop2;
			foreach (QilNode item2 in list)
			{
				qilNode = XPathBuilder.BuildOnePredicate(qilNode, item2, isReverseStep: false, f, predicateEnvironment.fixupVisitor, ref predicateEnvironment.numFixupCurrent, ref predicateEnvironment.numFixupPosition, ref predicateEnvironment.numFixupLast);
			}
			QilIterator qilIterator2 = f.For(qilNode);
			QilNode set = f.Filter(qilIterator2, f.Is(qilIterator2, variable));
			qilLoop.Body = f.Not(f.IsEmpty(set));
			qilLoop.Body = f.And(f.IsType(variable, qilLoop.XmlType), qilLoop.Body);
		}
		SetPriority(nodeset, 0.5);
		return nodeset;
	}

	public QilNode Function(string prefix, string name, IList<QilNode> args)
	{
		QilIterator qilIterator = f.For(fixupNode);
		QilNode binding = ((!(name == "id")) ? environment.ResolveFunction(prefix, name, args, new XsltFunctionFocus(qilIterator)) : f.Id(qilIterator, args[0]));
		QilIterator left;
		QilLoop qilLoop = f.BaseFactory.Filter(qilIterator, f.Not(f.IsEmpty(f.Filter(left = f.For(binding), f.Is(left, qilIterator)))));
		SetPriority(qilLoop, 0.5);
		SetLastParent(qilLoop, qilLoop);
		return qilLoop;
	}

	public QilNode String(string value)
	{
		return f.String(value);
	}

	public QilNode Number(double value)
	{
		return UnexpectedToken("Literal number");
	}

	public QilNode Variable(string prefix, string name)
	{
		return UnexpectedToken("Variable");
	}

	private QilNode UnexpectedToken(string tokenName)
	{
		throw new Exception(string.Format(CultureInfo.InvariantCulture, "Internal Error: {0} is not allowed in XSLT pattern outside of predicate.", tokenName));
	}

	public static void SetPriority(QilNode node, double priority)
	{
		Annotation annotation = ((Annotation)node.Annotation) ?? new Annotation();
		annotation.Priority = priority;
		node.Annotation = annotation;
	}

	public static double GetPriority(QilNode node)
	{
		return ((Annotation)node.Annotation).Priority;
	}

	private static void SetLastParent(QilNode node, QilLoop parent)
	{
		Annotation annotation = ((Annotation)node.Annotation) ?? new Annotation();
		annotation.Parent = parent;
		node.Annotation = annotation;
	}

	private static QilLoop GetLastParent(QilNode node)
	{
		return ((Annotation)node.Annotation).Parent;
	}

	public static void CleanAnnotation(QilNode node)
	{
		node.Annotation = null;
	}

	public IXPathBuilder<QilNode> GetPredicateBuilder(QilNode ctx)
	{
		_ = (QilLoop)ctx;
		return predicateBuilder;
	}
}
