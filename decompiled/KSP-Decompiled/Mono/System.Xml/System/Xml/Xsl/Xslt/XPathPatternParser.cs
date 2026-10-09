using System.Collections.Generic;
using System.Xml.XmlConfiguration;
using System.Xml.XPath;
using System.Xml.Xsl.Qil;
using System.Xml.Xsl.XPath;

namespace System.Xml.Xsl.Xslt;

internal class XPathPatternParser
{
	public interface IPatternBuilder : IXPathBuilder<QilNode>
	{
		IXPathBuilder<QilNode> GetPredicateBuilder(QilNode context);
	}

	private XPathScanner scanner;

	private IPatternBuilder ptrnBuilder;

	private XPathParser<QilNode> predicateParser = new XPathParser<QilNode>();

	private const int MaxParseRelativePathDepth = 1024;

	private int parseRelativePath;

	public QilNode Parse(XPathScanner scanner, IPatternBuilder ptrnBuilder)
	{
		QilNode result = null;
		ptrnBuilder.StartBuild();
		try
		{
			this.scanner = scanner;
			this.ptrnBuilder = ptrnBuilder;
			result = ParsePattern();
			this.scanner.CheckToken(LexKind.Eof);
		}
		finally
		{
			result = ptrnBuilder.EndBuild(result);
		}
		return result;
	}

	private QilNode ParsePattern()
	{
		QilNode qilNode = ParseLocationPathPattern();
		while (scanner.Kind == LexKind.Union)
		{
			scanner.NextLex();
			qilNode = ptrnBuilder.Operator(XPathOperator.Union, qilNode, ParseLocationPathPattern());
		}
		return qilNode;
	}

	private QilNode ParseLocationPathPattern()
	{
		switch (scanner.Kind)
		{
		case LexKind.Slash:
		{
			scanner.NextLex();
			QilNode qilNode = ptrnBuilder.Axis(XPathAxis.Root, XPathNodeType.All, null, null);
			if (XPathParser<QilNode>.IsStep(scanner.Kind))
			{
				qilNode = ptrnBuilder.JoinStep(qilNode, ParseRelativePathPattern());
			}
			return qilNode;
		}
		case LexKind.SlashSlash:
			scanner.NextLex();
			return ptrnBuilder.JoinStep(ptrnBuilder.Axis(XPathAxis.Root, XPathNodeType.All, null, null), ptrnBuilder.JoinStep(ptrnBuilder.Axis(XPathAxis.DescendantOrSelf, XPathNodeType.All, null, null), ParseRelativePathPattern()));
		case LexKind.Name:
			if (scanner.CanBeFunction && scanner.Prefix.Length == 0 && (scanner.Name == "id" || scanner.Name == "key"))
			{
				QilNode qilNode = ParseIdKeyPattern();
				switch (scanner.Kind)
				{
				case LexKind.Slash:
					scanner.NextLex();
					qilNode = ptrnBuilder.JoinStep(qilNode, ParseRelativePathPattern());
					break;
				case LexKind.SlashSlash:
					scanner.NextLex();
					qilNode = ptrnBuilder.JoinStep(qilNode, ptrnBuilder.JoinStep(ptrnBuilder.Axis(XPathAxis.DescendantOrSelf, XPathNodeType.All, null, null), ParseRelativePathPattern()));
					break;
				}
				return qilNode;
			}
			break;
		}
		return ParseRelativePathPattern();
	}

	private QilNode ParseIdKeyPattern()
	{
		List<QilNode> list = new List<QilNode>(2);
		if (scanner.Name == "id")
		{
			scanner.NextLex();
			scanner.PassToken(LexKind.LParens);
			scanner.CheckToken(LexKind.String);
			list.Add(ptrnBuilder.String(scanner.StringValue));
			scanner.NextLex();
			scanner.PassToken(LexKind.RParens);
			return ptrnBuilder.Function("", "id", list);
		}
		scanner.NextLex();
		scanner.PassToken(LexKind.LParens);
		scanner.CheckToken(LexKind.String);
		list.Add(ptrnBuilder.String(scanner.StringValue));
		scanner.NextLex();
		scanner.PassToken(LexKind.Comma);
		scanner.CheckToken(LexKind.String);
		list.Add(ptrnBuilder.String(scanner.StringValue));
		scanner.NextLex();
		scanner.PassToken(LexKind.RParens);
		return ptrnBuilder.Function("", "key", list);
	}

	private QilNode ParseRelativePathPattern()
	{
		if (++parseRelativePath > 1024 && XsltConfigSection.LimitXPathComplexity)
		{
			throw scanner.CreateException("The stylesheet is too complex.");
		}
		QilNode qilNode = ParseStepPattern();
		if (scanner.Kind == LexKind.Slash)
		{
			scanner.NextLex();
			qilNode = ptrnBuilder.JoinStep(qilNode, ParseRelativePathPattern());
		}
		else if (scanner.Kind == LexKind.SlashSlash)
		{
			scanner.NextLex();
			qilNode = ptrnBuilder.JoinStep(qilNode, ptrnBuilder.JoinStep(ptrnBuilder.Axis(XPathAxis.DescendantOrSelf, XPathNodeType.All, null, null), ParseRelativePathPattern()));
		}
		parseRelativePath--;
		return qilNode;
	}

	private QilNode ParseStepPattern()
	{
		XPathAxis xPathAxis;
		switch (scanner.Kind)
		{
		case LexKind.DotDot:
		case LexKind.Dot:
			throw scanner.CreateException("Only 'child' and 'attribute' axes are allowed in a pattern outside predicates.");
		case LexKind.At:
			xPathAxis = XPathAxis.Attribute;
			scanner.NextLex();
			break;
		case LexKind.Axis:
			xPathAxis = scanner.Axis;
			if (xPathAxis != XPathAxis.Child && xPathAxis != XPathAxis.Attribute)
			{
				throw scanner.CreateException("Only 'child' and 'attribute' axes are allowed in a pattern outside predicates.");
			}
			scanner.NextLex();
			scanner.NextLex();
			break;
		case LexKind.Name:
		case LexKind.Star:
			xPathAxis = XPathAxis.Child;
			break;
		default:
			throw scanner.CreateException("Unexpected token '{0}' in the expression.", scanner.RawValue);
		}
		XPathParser<QilNode>.InternalParseNodeTest(scanner, xPathAxis, out var nodeType, out var nodePrefix, out var nodeName);
		QilNode qilNode = ptrnBuilder.Axis(xPathAxis, nodeType, nodePrefix, nodeName);
		if (ptrnBuilder is XPathPatternBuilder xPathPatternBuilder)
		{
			List<QilNode> list = new List<QilNode>();
			while (scanner.Kind == LexKind.LBracket)
			{
				list.Add(ParsePredicate(qilNode));
			}
			if (list.Count > 0)
			{
				qilNode = xPathPatternBuilder.BuildPredicates(qilNode, list);
			}
		}
		else
		{
			while (scanner.Kind == LexKind.LBracket)
			{
				qilNode = ptrnBuilder.Predicate(qilNode, ParsePredicate(qilNode), reverseStep: false);
			}
		}
		return qilNode;
	}

	private QilNode ParsePredicate(QilNode context)
	{
		scanner.NextLex();
		QilNode result = predicateParser.Parse(scanner, ptrnBuilder.GetPredicateBuilder(context), LexKind.RBracket);
		scanner.NextLex();
		return result;
	}
}
