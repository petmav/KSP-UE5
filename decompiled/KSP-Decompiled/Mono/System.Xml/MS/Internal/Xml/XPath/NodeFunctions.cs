using System.Xml;
using System.Xml.XPath;
using System.Xml.Xsl;

namespace MS.Internal.Xml.XPath;

internal sealed class NodeFunctions : ValueQuery
{
	private Query arg;

	private Function.FunctionType funcType;

	private XsltContext xsltContext;

	public override XPathResultType StaticType => Function.ReturnTypes[(int)funcType];

	public NodeFunctions(Function.FunctionType funcType, Query arg)
	{
		this.funcType = funcType;
		this.arg = arg;
	}

	public override void SetXsltContext(XsltContext context)
	{
		xsltContext = (context.Whitespace ? context : null);
		if (arg != null)
		{
			arg.SetXsltContext(context);
		}
	}

	private XPathNavigator EvaluateArg(XPathNodeIterator context)
	{
		if (arg == null)
		{
			return context.Current;
		}
		arg.Evaluate(context);
		return arg.Advance();
	}

	public override object Evaluate(XPathNodeIterator context)
	{
		switch (funcType)
		{
		case Function.FunctionType.FuncPosition:
			return (double)context.CurrentPosition;
		case Function.FunctionType.FuncLast:
			return (double)context.Count;
		case Function.FunctionType.FuncNameSpaceUri:
		{
			XPathNavigator xPathNavigator2 = EvaluateArg(context);
			if (xPathNavigator2 != null)
			{
				return xPathNavigator2.NamespaceURI;
			}
			break;
		}
		case Function.FunctionType.FuncLocalName:
		{
			XPathNavigator xPathNavigator2 = EvaluateArg(context);
			if (xPathNavigator2 != null)
			{
				return xPathNavigator2.LocalName;
			}
			break;
		}
		case Function.FunctionType.FuncName:
		{
			XPathNavigator xPathNavigator2 = EvaluateArg(context);
			if (xPathNavigator2 != null)
			{
				return xPathNavigator2.Name;
			}
			break;
		}
		case Function.FunctionType.FuncCount:
		{
			arg.Evaluate(context);
			int num = 0;
			if (xsltContext != null)
			{
				XPathNavigator xPathNavigator;
				while ((xPathNavigator = arg.Advance()) != null)
				{
					if (xPathNavigator.NodeType != XPathNodeType.Whitespace || xsltContext.PreserveWhitespace(xPathNavigator))
					{
						num++;
					}
				}
			}
			else
			{
				while (arg.Advance() != null)
				{
					num++;
				}
			}
			return (double)num;
		}
		}
		return string.Empty;
	}

	public override XPathNodeIterator Clone()
	{
		return new NodeFunctions(funcType, Query.Clone(arg))
		{
			xsltContext = xsltContext
		};
	}

	public override void PrintQuery(XmlWriter w)
	{
		w.WriteStartElement(GetType().Name);
		w.WriteAttributeString("name", funcType.ToString());
		if (arg != null)
		{
			arg.PrintQuery(w);
		}
		w.WriteEndElement();
	}
}
