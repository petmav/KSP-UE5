using System.Collections.Generic;
using System.Xml.Xsl.Qil;

namespace System.Xml.Xsl.Xslt;

internal sealed class XslAstRewriter
{
	private static readonly QilName nullMode = AstFactory.QName(string.Empty);

	private CompilerScopeManager<VarPar> scope;

	private Stack<Template> newTemplates;

	private Compiler compiler;

	private const int FixedNodeCost = 1;

	private const int IteratorNodeCost = 2;

	private const int CallTemplateCost = 1;

	private const int RewriteThreshold = 100;

	private const int NodesWithSelect = -247451132;

	private const int ParentsOfCallTemplate = -1025034872;

	public void Rewrite(Compiler compiler)
	{
		this.compiler = compiler;
		scope = new CompilerScopeManager<VarPar>();
		newTemplates = new Stack<Template>();
		foreach (ProtoTemplate allTemplate in compiler.AllTemplates)
		{
			scope.EnterScope();
			CheckNodeCost(allTemplate);
		}
		while (newTemplates.Count > 0)
		{
			Template template = newTemplates.Pop();
			compiler.AllTemplates.Add(template);
			compiler.NamedTemplates.Add(template.Name, template);
			scope.EnterScope();
			CheckNodeCost(template);
		}
	}

	private static int NodeCostForXPath(string xpath)
	{
		int num = 0;
		if (xpath != null)
		{
			num = 2;
			for (int i = 2; i < xpath.Length; i += 2)
			{
				if (xpath[i] == '/' || xpath[i - 1] == '/')
				{
					num += 2;
				}
			}
		}
		return num;
	}

	private static bool NodeTypeTest(XslNodeType nodetype, int flags)
	{
		return ((flags >> (int)nodetype) & 1) != 0;
	}

	private int CheckNodeCost(XslNode node)
	{
		scope.EnterScope(node.Namespaces);
		bool flag = false;
		int num = 1;
		if (NodeTypeTest(node.NodeType, -247451132))
		{
			num += NodeCostForXPath(node.Select);
		}
		IList<XslNode> content = node.Content;
		int num2 = content.Count - 1;
		for (int i = 0; i <= num2; i++)
		{
			XslNode xslNode = content[i];
			int num3 = CheckNodeCost(xslNode);
			num += num3;
			if (flag && num > 100)
			{
				if (i < num2 || num3 > 1)
				{
					Refactor(node, i);
					num -= num3;
					num++;
				}
				break;
			}
			if (xslNode.NodeType == XslNodeType.Variable || xslNode.NodeType == XslNodeType.Param)
			{
				scope.AddVariable(xslNode.Name, (VarPar)xslNode);
				if (xslNode.NodeType == XslNodeType.Param)
				{
					num -= num3;
				}
			}
			else if (!flag)
			{
				flag = NodeTypeTest(node.NodeType, -1025034872);
			}
		}
		scope.ExitScope();
		return num;
	}

	private void Refactor(XslNode parent, int split)
	{
		List<XslNode> list = (List<XslNode>)parent.Content;
		XslNode xslNode = list[split];
		QilName name = AstFactory.QName("generated", compiler.CreatePhantomNamespace(), "compiler");
		XsltInput.ContextInfo contextInfo = new XsltInput.ContextInfo(xslNode.SourceLine);
		XslNodeEx xslNodeEx = AstFactory.CallTemplate(name, contextInfo);
		XsltLoader.SetInfo(xslNodeEx, null, contextInfo);
		Template template = AstFactory.Template(name, null, XsltLoader.nullMode, double.NaN, xslNode.XslVersion);
		XsltLoader.SetInfo(template, null, contextInfo);
		newTemplates.Push(template);
		template.SetContent(new List<XslNode>(list.Count - split + 8));
		foreach (CompilerScopeManager<VarPar>.ScopeRecord activeRecord in scope.GetActiveRecords())
		{
			if (!activeRecord.IsVariable)
			{
				template.Namespaces = new NsDecl(template.Namespaces, activeRecord.ncName, activeRecord.nsUri);
				continue;
			}
			VarPar value = activeRecord.value;
			if (!compiler.IsPhantomNamespace(value.Name.NamespaceUri))
			{
				QilName qilName = AstFactory.QName(value.Name.LocalName, value.Name.NamespaceUri, value.Name.Prefix);
				VarPar varPar = AstFactory.VarPar(XslNodeType.WithParam, qilName, "$" + qilName.QualifiedName, XslVersion.Version10);
				XsltLoader.SetInfo(varPar, null, contextInfo);
				varPar.Namespaces = value.Namespaces;
				xslNodeEx.AddContent(varPar);
				VarPar varPar2 = AstFactory.VarPar(XslNodeType.Param, qilName, null, XslVersion.Version10);
				XsltLoader.SetInfo(varPar2, null, contextInfo);
				varPar2.Namespaces = value.Namespaces;
				template.AddContent(varPar2);
			}
		}
		for (int i = split; i < list.Count; i++)
		{
			template.AddContent(list[i]);
		}
		list[split] = xslNodeEx;
		list.RemoveRange(split + 1, list.Count - split - 1);
	}
}
