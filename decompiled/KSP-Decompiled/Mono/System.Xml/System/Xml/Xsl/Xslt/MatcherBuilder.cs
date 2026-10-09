using System.Collections.Generic;
using System.Xml.Xsl.Qil;
using System.Xml.Xsl.XPath;

namespace System.Xml.Xsl.Xslt;

internal class MatcherBuilder
{
	private XPathQilFactory f;

	private ReferenceReplacer refReplacer;

	private InvokeGenerator invkGen;

	private const int NoMatch = -1;

	private int priority = -1;

	private PatternBag elementPatterns = new PatternBag();

	private PatternBag attributePatterns = new PatternBag();

	private List<Pattern> textPatterns = new List<Pattern>();

	private List<Pattern> documentPatterns = new List<Pattern>();

	private List<Pattern> commentPatterns = new List<Pattern>();

	private PatternBag piPatterns = new PatternBag();

	private List<Pattern> heterogenousPatterns = new List<Pattern>();

	private List<List<TemplateMatch>> allMatches = new List<List<TemplateMatch>>();

	public MatcherBuilder(XPathQilFactory f, ReferenceReplacer refReplacer, InvokeGenerator invkGen)
	{
		this.f = f;
		this.refReplacer = refReplacer;
		this.invkGen = invkGen;
	}

	private void Clear()
	{
		priority = -1;
		elementPatterns.Clear();
		attributePatterns.Clear();
		textPatterns.Clear();
		documentPatterns.Clear();
		commentPatterns.Clear();
		piPatterns.Clear();
		heterogenousPatterns.Clear();
		allMatches.Clear();
	}

	private void AddPatterns(List<TemplateMatch> matches)
	{
		foreach (TemplateMatch match in matches)
		{
			Pattern pattern = new Pattern(match, ++priority);
			switch (match.NodeKind)
			{
			case XmlNodeKindFlags.Element:
				elementPatterns.Add(pattern);
				break;
			case XmlNodeKindFlags.Attribute:
				attributePatterns.Add(pattern);
				break;
			case XmlNodeKindFlags.Text:
				textPatterns.Add(pattern);
				break;
			case XmlNodeKindFlags.Document:
				documentPatterns.Add(pattern);
				break;
			case XmlNodeKindFlags.Comment:
				commentPatterns.Add(pattern);
				break;
			case XmlNodeKindFlags.PI:
				piPatterns.Add(pattern);
				break;
			default:
				heterogenousPatterns.Add(pattern);
				break;
			}
		}
	}

	private void CollectPatternsInternal(Stylesheet sheet, QilName mode)
	{
		Stylesheet[] imports = sheet.Imports;
		foreach (Stylesheet sheet2 in imports)
		{
			CollectPatternsInternal(sheet2, mode);
		}
		if (sheet.TemplateMatches.TryGetValue(mode, out var value))
		{
			AddPatterns(value);
			allMatches.Add(value);
		}
	}

	public void CollectPatterns(StylesheetLevel sheet, QilName mode)
	{
		Clear();
		Stylesheet[] imports = sheet.Imports;
		foreach (Stylesheet sheet2 in imports)
		{
			CollectPatternsInternal(sheet2, mode);
		}
	}

	private QilNode MatchPattern(QilIterator it, TemplateMatch match)
	{
		QilNode condition = match.Condition;
		if (condition == null)
		{
			return f.True();
		}
		condition = condition.DeepClone(f.BaseFactory);
		return refReplacer.Replace(condition, match.Iterator, it);
	}

	private QilNode MatchPatterns(QilIterator it, List<Pattern> patternList)
	{
		QilNode qilNode = f.Int32(-1);
		foreach (Pattern pattern in patternList)
		{
			qilNode = f.Conditional(MatchPattern(it, pattern.Match), f.Int32(pattern.Priority), qilNode);
		}
		return qilNode;
	}

	private QilNode MatchPatterns(QilIterator it, XmlQueryType xt, List<Pattern> patternList, QilNode otherwise)
	{
		if (patternList.Count == 0)
		{
			return otherwise;
		}
		return f.Conditional(f.IsType(it, xt), MatchPatterns(it, patternList), otherwise);
	}

	private bool IsNoMatch(QilNode matcher)
	{
		if (matcher.NodeType == QilNodeType.LiteralInt32)
		{
			return true;
		}
		return false;
	}

	private QilNode MatchPatternsWhosePriorityGreater(QilIterator it, List<Pattern> patternList, QilNode matcher)
	{
		if (patternList.Count == 0)
		{
			return matcher;
		}
		if (IsNoMatch(matcher))
		{
			return MatchPatterns(it, patternList);
		}
		QilIterator qilIterator = f.Let(matcher);
		QilNode qilNode = f.Int32(-1);
		int num = -1;
		foreach (Pattern pattern in patternList)
		{
			if (pattern.Priority > num + 1)
			{
				qilNode = f.Conditional(f.Gt(qilIterator, f.Int32(num)), qilIterator, qilNode);
			}
			qilNode = f.Conditional(MatchPattern(it, pattern.Match), f.Int32(pattern.Priority), qilNode);
			num = pattern.Priority;
		}
		if (num != priority)
		{
			qilNode = f.Conditional(f.Gt(qilIterator, f.Int32(num)), qilIterator, qilNode);
		}
		return f.Loop(qilIterator, qilNode);
	}

	private QilNode MatchPatterns(QilIterator it, XmlQueryType xt, PatternBag patternBag, QilNode otherwise)
	{
		if (patternBag.FixedNamePatternsNames.Count == 0)
		{
			return MatchPatterns(it, xt, patternBag.NonFixedNamePatterns, otherwise);
		}
		QilNode qilNode = f.Int32(-1);
		foreach (QilName fixedNamePatternsName in patternBag.FixedNamePatternsNames)
		{
			qilNode = f.Conditional(f.Eq(f.NameOf(it), fixedNamePatternsName.ShallowClone(f.BaseFactory)), MatchPatterns(it, patternBag.FixedNamePatterns[fixedNamePatternsName]), qilNode);
		}
		qilNode = MatchPatternsWhosePriorityGreater(it, patternBag.NonFixedNamePatterns, qilNode);
		return f.Conditional(f.IsType(it, xt), qilNode, otherwise);
	}

	public QilNode BuildMatcher(QilIterator it, IList<XslNode> actualArgs, QilNode otherwise)
	{
		QilNode otherwise2 = f.Int32(-1);
		otherwise2 = MatchPatterns(it, XmlQueryTypeFactory.PI, piPatterns, otherwise2);
		otherwise2 = MatchPatterns(it, XmlQueryTypeFactory.Comment, commentPatterns, otherwise2);
		otherwise2 = MatchPatterns(it, XmlQueryTypeFactory.Document, documentPatterns, otherwise2);
		otherwise2 = MatchPatterns(it, XmlQueryTypeFactory.Text, textPatterns, otherwise2);
		otherwise2 = MatchPatterns(it, XmlQueryTypeFactory.Attribute, attributePatterns, otherwise2);
		otherwise2 = MatchPatterns(it, XmlQueryTypeFactory.Element, elementPatterns, otherwise2);
		otherwise2 = MatchPatternsWhosePriorityGreater(it, heterogenousPatterns, otherwise2);
		if (IsNoMatch(otherwise2))
		{
			return otherwise;
		}
		QilNode[] array = new QilNode[priority + 2];
		int num = -1;
		foreach (List<TemplateMatch> allMatch in allMatches)
		{
			foreach (TemplateMatch item in allMatch)
			{
				array[++num] = invkGen.GenerateInvoke(item.TemplateFunction, actualArgs);
			}
		}
		array[++num] = otherwise;
		return f.Choice(otherwise2, f.BranchList(array));
	}
}
