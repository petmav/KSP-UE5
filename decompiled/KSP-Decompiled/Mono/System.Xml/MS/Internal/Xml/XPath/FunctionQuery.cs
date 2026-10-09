using System;
using System.Collections.Generic;
using System.Xml;
using System.Xml.XPath;
using System.Xml.Xsl;

namespace MS.Internal.Xml.XPath;

internal sealed class FunctionQuery : ExtensionQuery
{
	private IList<Query> args;

	private IXsltContextFunction function;

	public override XPathResultType StaticType
	{
		get
		{
			XPathResultType xPathResultType = ((function != null) ? function.ReturnType : XPathResultType.Any);
			if (xPathResultType == XPathResultType.Error)
			{
				xPathResultType = XPathResultType.Any;
			}
			return xPathResultType;
		}
	}

	public FunctionQuery(string prefix, string name, List<Query> args)
		: base(prefix, name)
	{
		this.args = args;
	}

	private FunctionQuery(FunctionQuery other)
		: base(other)
	{
		function = other.function;
		Query[] array = new Query[other.args.Count];
		for (int i = 0; i < array.Length; i++)
		{
			array[i] = Query.Clone(other.args[i]);
		}
		args = array;
		args = array;
	}

	public override void SetXsltContext(XsltContext context)
	{
		if (context == null)
		{
			throw XPathException.Create("Namespace Manager or XsltContext needed. This query has a prefix, variable, or user-defined function.");
		}
		if (xsltContext == context)
		{
			return;
		}
		xsltContext = context;
		foreach (Query arg in args)
		{
			arg.SetXsltContext(context);
		}
		XPathResultType[] array = new XPathResultType[args.Count];
		for (int i = 0; i < args.Count; i++)
		{
			array[i] = args[i].StaticType;
		}
		function = xsltContext.ResolveFunction(prefix, name, array);
		if (function == null)
		{
			throw XPathException.Create("The function '{0}()' is undefined.", base.QName);
		}
	}

	public override object Evaluate(XPathNodeIterator nodeIterator)
	{
		if (xsltContext == null)
		{
			throw XPathException.Create("Namespace Manager or XsltContext needed. This query has a prefix, variable, or user-defined function.");
		}
		object[] array = new object[args.Count];
		for (int i = 0; i < args.Count; i++)
		{
			array[i] = args[i].Evaluate(nodeIterator);
			if (array[i] is XPathNodeIterator)
			{
				array[i] = new XPathSelectionIterator(nodeIterator.Current, args[i]);
			}
		}
		try
		{
			return ProcessResult(function.Invoke(xsltContext, array, nodeIterator.Current));
		}
		catch (Exception innerException)
		{
			throw XPathException.Create("Function '{0}()' has failed.", base.QName, innerException);
		}
	}

	public override XPathNavigator MatchNode(XPathNavigator navigator)
	{
		if (name != "key" && prefix.Length != 0)
		{
			throw XPathException.Create("'{0}' is an invalid XSLT pattern.");
		}
		Evaluate(new XPathSingletonIterator(navigator, moved: true));
		XPathNavigator xPathNavigator = null;
		while ((xPathNavigator = Advance()) != null)
		{
			if (xPathNavigator.IsSamePosition(navigator))
			{
				return xPathNavigator;
			}
		}
		return xPathNavigator;
	}

	public override XPathNodeIterator Clone()
	{
		return new FunctionQuery(this);
	}

	public override void PrintQuery(XmlWriter w)
	{
		w.WriteStartElement(GetType().Name);
		w.WriteAttributeString("name", (prefix.Length != 0) ? (prefix + ":" + name) : name);
		foreach (Query arg in args)
		{
			arg.PrintQuery(w);
		}
		w.WriteEndElement();
	}
}
