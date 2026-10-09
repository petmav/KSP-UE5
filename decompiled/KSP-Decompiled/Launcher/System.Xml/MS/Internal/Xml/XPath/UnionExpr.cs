using System;
using System.Xml;
using System.Xml.XPath;
using System.Xml.Xsl;

namespace MS.Internal.Xml.XPath;

internal sealed class UnionExpr : Query
{
	internal Query qy1;

	internal Query qy2;

	private bool advance1;

	private bool advance2;

	private XPathNavigator currentNode;

	private XPathNavigator nextNode;

	public override XPathResultType StaticType => XPathResultType.NodeSet;

	public override XPathNavigator Current => currentNode;

	public override int CurrentPosition
	{
		get
		{
			throw new InvalidOperationException();
		}
	}

	public UnionExpr(Query query1, Query query2)
	{
		qy1 = query1;
		qy2 = query2;
		advance1 = true;
		advance2 = true;
	}

	private UnionExpr(UnionExpr other)
		: base(other)
	{
		qy1 = Query.Clone(other.qy1);
		qy2 = Query.Clone(other.qy2);
		advance1 = other.advance1;
		advance2 = other.advance2;
		currentNode = Query.Clone(other.currentNode);
		nextNode = Query.Clone(other.nextNode);
	}

	public override void Reset()
	{
		qy1.Reset();
		qy2.Reset();
		advance1 = true;
		advance2 = true;
		nextNode = null;
	}

	public override void SetXsltContext(XsltContext xsltContext)
	{
		qy1.SetXsltContext(xsltContext);
		qy2.SetXsltContext(xsltContext);
	}

	public override object Evaluate(XPathNodeIterator context)
	{
		qy1.Evaluate(context);
		qy2.Evaluate(context);
		advance1 = true;
		advance2 = true;
		nextNode = null;
		ResetCount();
		return this;
	}

	private XPathNavigator ProcessSamePosition(XPathNavigator result)
	{
		currentNode = result;
		advance1 = (advance2 = true);
		return result;
	}

	private XPathNavigator ProcessBeforePosition(XPathNavigator res1, XPathNavigator res2)
	{
		nextNode = res2;
		advance2 = false;
		advance1 = true;
		currentNode = res1;
		return res1;
	}

	private XPathNavigator ProcessAfterPosition(XPathNavigator res1, XPathNavigator res2)
	{
		nextNode = res1;
		advance1 = false;
		advance2 = true;
		currentNode = res2;
		return res2;
	}

	public override XPathNavigator Advance()
	{
		XmlNodeOrder xmlNodeOrder = XmlNodeOrder.Before;
		XPathNavigator xPathNavigator = ((!advance1) ? nextNode : qy1.Advance());
		XPathNavigator xPathNavigator2 = ((!advance2) ? nextNode : qy2.Advance());
		if (xPathNavigator == null || xPathNavigator2 == null)
		{
			if (xPathNavigator2 == null)
			{
				advance1 = true;
				advance2 = false;
				currentNode = xPathNavigator;
				nextNode = null;
				return xPathNavigator;
			}
			advance1 = false;
			advance2 = true;
			currentNode = xPathNavigator2;
			nextNode = null;
			return xPathNavigator2;
		}
		return Query.CompareNodes(xPathNavigator, xPathNavigator2) switch
		{
			XmlNodeOrder.Before => ProcessBeforePosition(xPathNavigator, xPathNavigator2), 
			XmlNodeOrder.After => ProcessAfterPosition(xPathNavigator, xPathNavigator2), 
			_ => ProcessSamePosition(xPathNavigator), 
		};
	}

	public override XPathNavigator MatchNode(XPathNavigator xsltContext)
	{
		if (xsltContext != null)
		{
			XPathNavigator xPathNavigator = qy1.MatchNode(xsltContext);
			if (xPathNavigator != null)
			{
				return xPathNavigator;
			}
			return qy2.MatchNode(xsltContext);
		}
		return null;
	}

	public override XPathNodeIterator Clone()
	{
		return new UnionExpr(this);
	}

	public override void PrintQuery(XmlWriter w)
	{
		w.WriteStartElement(GetType().Name);
		if (qy1 != null)
		{
			qy1.PrintQuery(w);
		}
		if (qy2 != null)
		{
			qy2.PrintQuery(w);
		}
		w.WriteEndElement();
	}
}
