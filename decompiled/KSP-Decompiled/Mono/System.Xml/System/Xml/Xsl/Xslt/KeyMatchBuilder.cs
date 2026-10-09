using System.Xml.Xsl.Qil;
using System.Xml.Xsl.XPath;

namespace System.Xml.Xsl.Xslt;

internal class KeyMatchBuilder : XPathBuilder, XPathPatternParser.IPatternBuilder, IXPathBuilder<QilNode>
{
	internal class PathConvertor : QilReplaceVisitor
	{
		private new XPathQilFactory f;

		private QilNode fixup;

		public PathConvertor(XPathQilFactory f)
			: base(f.BaseFactory)
		{
			this.f = f;
		}

		public QilNode ConvertReletive2Absolute(QilNode node, QilNode fixup)
		{
			QilDepthChecker.Check(node);
			this.fixup = fixup;
			return Visit(node);
		}

		protected override QilNode Visit(QilNode n)
		{
			if (n.NodeType == QilNodeType.Union || n.NodeType == QilNodeType.DocOrderDistinct || n.NodeType == QilNodeType.Filter || n.NodeType == QilNodeType.Loop)
			{
				return base.Visit(n);
			}
			return n;
		}

		protected override QilNode VisitLoop(QilLoop n)
		{
			if (n.Variable.Binding.NodeType == QilNodeType.Root || n.Variable.Binding.NodeType == QilNodeType.Deref)
			{
				return n;
			}
			if (n.Variable.Binding.NodeType == QilNodeType.Content)
			{
				QilUnary qilUnary = (QilUnary)n.Variable.Binding;
				QilIterator variable = (QilIterator)(qilUnary.Child = f.For(f.DescendantOrSelf(f.Root(fixup))));
				n.Variable.Binding = f.Loop(variable, qilUnary);
				return n;
			}
			n.Variable.Binding = Visit(n.Variable.Binding);
			return n;
		}

		protected override QilNode VisitFilter(QilLoop n)
		{
			return VisitLoop(n);
		}
	}

	private int depth;

	private PathConvertor convertor;

	public KeyMatchBuilder(IXPathEnvironment env)
		: base(env)
	{
		convertor = new PathConvertor(env.Factory);
	}

	public override void StartBuild()
	{
		if (depth == 0)
		{
			base.StartBuild();
		}
		depth++;
	}

	public override QilNode EndBuild(QilNode result)
	{
		depth--;
		if (result == null)
		{
			return base.EndBuild(result);
		}
		if (depth == 0)
		{
			result = convertor.ConvertReletive2Absolute(result, fixupCurrent);
			result = base.EndBuild(result);
		}
		return result;
	}

	public virtual IXPathBuilder<QilNode> GetPredicateBuilder(QilNode ctx)
	{
		return this;
	}
}
