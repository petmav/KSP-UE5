using System.Collections;
using System.Xml;
using System.Xml.XPath;
using System.Xml.Xsl;

namespace MS.Internal.Xml.XPath;

internal class CompiledXpathExpr : XPathExpression
{
	private class UndefinedXsltContext : XsltContext
	{
		private IXmlNamespaceResolver nsResolver;

		public override string DefaultNamespace => string.Empty;

		public override bool Whitespace => false;

		public UndefinedXsltContext(IXmlNamespaceResolver nsResolver)
			: base(dummy: false)
		{
			this.nsResolver = nsResolver;
		}

		public override string LookupNamespace(string prefix)
		{
			if (prefix.Length == 0)
			{
				return string.Empty;
			}
			return nsResolver.LookupNamespace(prefix) ?? throw XPathException.Create("Namespace prefix '{0}' is not defined.", prefix);
		}

		public override IXsltContextVariable ResolveVariable(string prefix, string name)
		{
			throw XPathException.Create("XsltContext is needed for this query because of an unknown function.");
		}

		public override IXsltContextFunction ResolveFunction(string prefix, string name, XPathResultType[] ArgTypes)
		{
			throw XPathException.Create("XsltContext is needed for this query because of an unknown function.");
		}

		public override bool PreserveWhitespace(XPathNavigator node)
		{
			return false;
		}

		public override int CompareDocument(string baseUri, string nextbaseUri)
		{
			return string.CompareOrdinal(baseUri, nextbaseUri);
		}
	}

	private Query query;

	private string expr;

	private bool needContext;

	internal Query QueryTree
	{
		get
		{
			if (needContext)
			{
				throw XPathException.Create("Namespace Manager or XsltContext needed. This query has a prefix, variable, or user-defined function.");
			}
			return query;
		}
	}

	public override string Expression => expr;

	public override XPathResultType ReturnType => query.StaticType;

	internal CompiledXpathExpr(Query query, string expression, bool needContext)
	{
		this.query = query;
		expr = expression;
		this.needContext = needContext;
	}

	public virtual void CheckErrors()
	{
	}

	public override void AddSort(object expr, IComparer comparer)
	{
		Query evalQuery;
		if (expr is string)
		{
			evalQuery = new QueryBuilder().Build((string)expr, out needContext);
		}
		else
		{
			if (!(expr is CompiledXpathExpr))
			{
				throw XPathException.Create("This is an invalid object. Only objects returned from Compile() can be passed as input.");
			}
			evalQuery = ((CompiledXpathExpr)expr).QueryTree;
		}
		SortQuery sortQuery = query as SortQuery;
		if (sortQuery == null)
		{
			sortQuery = (SortQuery)(query = new SortQuery(query));
		}
		sortQuery.AddSort(evalQuery, comparer);
	}

	public override void AddSort(object expr, XmlSortOrder order, XmlCaseOrder caseOrder, string lang, XmlDataType dataType)
	{
		AddSort(expr, new XPathComparerHelper(order, caseOrder, lang, dataType));
	}

	public override XPathExpression Clone()
	{
		return new CompiledXpathExpr(Query.Clone(query), expr, needContext);
	}

	public override void SetContext(XmlNamespaceManager nsManager)
	{
		SetContext((IXmlNamespaceResolver)nsManager);
	}

	public override void SetContext(IXmlNamespaceResolver nsResolver)
	{
		XsltContext xsltContext = nsResolver as XsltContext;
		if (xsltContext == null)
		{
			if (nsResolver == null)
			{
				nsResolver = new XmlNamespaceManager(new NameTable());
			}
			xsltContext = new UndefinedXsltContext(nsResolver);
		}
		query.SetXsltContext(xsltContext);
		needContext = false;
	}
}
