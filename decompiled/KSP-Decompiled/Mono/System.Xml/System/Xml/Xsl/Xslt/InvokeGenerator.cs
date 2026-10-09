using System.Collections.Generic;
using System.Xml.Xsl.Qil;

namespace System.Xml.Xsl.Xslt;

internal class InvokeGenerator : QilCloneVisitor
{
	private bool debug;

	private Stack<QilIterator> iterStack;

	private QilList formalArgs;

	private QilList invokeArgs;

	private int curArg;

	private XsltQilFactory fac;

	public InvokeGenerator(XsltQilFactory f, bool debug)
		: base(f.BaseFactory)
	{
		this.debug = debug;
		fac = f;
		iterStack = new Stack<QilIterator>();
	}

	public QilNode GenerateInvoke(QilFunction func, IList<XslNode> actualArgs)
	{
		iterStack.Clear();
		formalArgs = func.Arguments;
		invokeArgs = fac.ActualParameterList();
		for (curArg = 0; curArg < formalArgs.Count; curArg++)
		{
			QilParameter qilParameter = (QilParameter)formalArgs[curArg];
			QilNode qilNode = FindActualArg(qilParameter, actualArgs);
			if (qilNode == null)
			{
				qilNode = ((!debug) ? Clone(qilParameter.DefaultValue) : ((!(qilParameter.Name.NamespaceUri == "urn:schemas-microsoft-com:xslt-debug")) ? fac.DefaultValueMarker() : Clone(qilParameter.DefaultValue)));
			}
			XmlQueryType xmlType = qilParameter.XmlType;
			if (!qilNode.XmlType.IsSubtypeOf(xmlType))
			{
				qilNode = fac.TypeAssert(qilNode, xmlType);
			}
			invokeArgs.Add(qilNode);
		}
		QilNode qilNode2 = fac.Invoke(func, invokeArgs);
		while (iterStack.Count != 0)
		{
			qilNode2 = fac.Loop(iterStack.Pop(), qilNode2);
		}
		return qilNode2;
	}

	private QilNode FindActualArg(QilParameter formalArg, IList<XslNode> actualArgs)
	{
		QilName name = formalArg.Name;
		foreach (XslNode actualArg in actualArgs)
		{
			if (actualArg.Name.Equals(name))
			{
				return ((VarPar)actualArg).Value;
			}
		}
		return null;
	}

	protected override QilNode VisitReference(QilNode n)
	{
		QilNode qilNode = FindClonedReference(n);
		if (qilNode != null)
		{
			return qilNode;
		}
		for (int i = 0; i < curArg; i++)
		{
			if (n == formalArgs[i])
			{
				if (invokeArgs[i] is QilLiteral)
				{
					return invokeArgs[i].ShallowClone(fac.BaseFactory);
				}
				if (!(invokeArgs[i] is QilIterator))
				{
					QilIterator qilIterator = fac.BaseFactory.Let(invokeArgs[i]);
					iterStack.Push(qilIterator);
					invokeArgs[i] = qilIterator;
				}
				return invokeArgs[i];
			}
		}
		return n;
	}

	protected override QilNode VisitFunction(QilFunction n)
	{
		return n;
	}
}
