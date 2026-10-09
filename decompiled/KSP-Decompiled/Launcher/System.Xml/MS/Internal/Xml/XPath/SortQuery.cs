using System.Collections;
using System.Collections.Generic;
using System.Xml;
using System.Xml.XPath;
using System.Xml.Xsl;

namespace MS.Internal.Xml.XPath;

internal sealed class SortQuery : Query
{
	private List<SortKey> results;

	private XPathSortComparer comparer;

	private Query qyInput;

	public override XPathNavigator Current
	{
		get
		{
			if (count == 0)
			{
				return null;
			}
			return results[count - 1].Node;
		}
	}

	public override XPathResultType StaticType => XPathResultType.NodeSet;

	public override int CurrentPosition => count;

	public override int Count => results.Count;

	public override QueryProps Properties => (QueryProps)7;

	public SortQuery(Query qyInput)
	{
		results = new List<SortKey>();
		comparer = new XPathSortComparer();
		this.qyInput = qyInput;
		count = 0;
	}

	private SortQuery(SortQuery other)
		: base(other)
	{
		results = new List<SortKey>(other.results);
		comparer = other.comparer.Clone();
		qyInput = Query.Clone(other.qyInput);
		count = 0;
	}

	public override void Reset()
	{
		count = 0;
	}

	public override void SetXsltContext(XsltContext xsltContext)
	{
		qyInput.SetXsltContext(xsltContext);
		if (qyInput.StaticType != XPathResultType.NodeSet && qyInput.StaticType != XPathResultType.Any)
		{
			throw XPathException.Create("Expression must evaluate to a node-set.");
		}
	}

	private void BuildResultsList()
	{
		int numSorts = comparer.NumSorts;
		XPathNavigator xPathNavigator;
		while ((xPathNavigator = qyInput.Advance()) != null)
		{
			SortKey sortKey = new SortKey(numSorts, results.Count, xPathNavigator.Clone());
			for (int i = 0; i < numSorts; i++)
			{
				sortKey[i] = comparer.Expression(i).Evaluate(qyInput);
			}
			results.Add(sortKey);
		}
		results.Sort(comparer);
	}

	public override object Evaluate(XPathNodeIterator context)
	{
		qyInput.Evaluate(context);
		results.Clear();
		BuildResultsList();
		count = 0;
		return this;
	}

	public override XPathNavigator Advance()
	{
		if (count < results.Count)
		{
			return results[count++].Node;
		}
		return null;
	}

	internal void AddSort(Query evalQuery, IComparer comparer)
	{
		this.comparer.AddSort(evalQuery, comparer);
	}

	public override XPathNodeIterator Clone()
	{
		return new SortQuery(this);
	}

	public override void PrintQuery(XmlWriter w)
	{
		w.WriteStartElement(GetType().Name);
		qyInput.PrintQuery(w);
		w.WriteElementString("XPathSortComparer", "... PrintTree() not implemented ...");
		w.WriteEndElement();
	}
}
