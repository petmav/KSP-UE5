using System.Xml.XPath;
using System.Xml.Xsl.Qil;

namespace System.Xml.Xsl.IlGen;

internal class XmlILNamespaceAnalyzer
{
	private XmlNamespaceManager nsmgr = new XmlNamespaceManager(new NameTable());

	private bool addInScopeNmsp;

	private int cntNmsp;

	public void Analyze(QilNode nd, bool defaultNmspInScope)
	{
		addInScopeNmsp = false;
		cntNmsp = 0;
		if (defaultNmspInScope)
		{
			nsmgr.PushScope();
			nsmgr.AddNamespace(string.Empty, string.Empty);
			cntNmsp++;
		}
		AnalyzeContent(nd);
		if (defaultNmspInScope)
		{
			nsmgr.PopScope();
		}
	}

	private void AnalyzeContent(QilNode nd)
	{
		switch (nd.NodeType)
		{
		case QilNodeType.Loop:
			addInScopeNmsp = false;
			AnalyzeContent((nd as QilLoop).Body);
			break;
		case QilNodeType.Sequence:
		{
			foreach (QilNode item in nd)
			{
				AnalyzeContent(item);
			}
			break;
		}
		case QilNodeType.Conditional:
			addInScopeNmsp = false;
			AnalyzeContent((nd as QilTernary).Center);
			AnalyzeContent((nd as QilTernary).Right);
			break;
		case QilNodeType.Choice:
		{
			addInScopeNmsp = false;
			QilList branches = (nd as QilChoice).Branches;
			for (int i = 0; i < branches.Count; i++)
			{
				AnalyzeContent(branches[i]);
			}
			break;
		}
		case QilNodeType.ElementCtor:
		{
			addInScopeNmsp = true;
			nsmgr.PushScope();
			int num = cntNmsp;
			if (CheckNamespaceInScope(nd as QilBinary))
			{
				AnalyzeContent((nd as QilBinary).Right);
			}
			nsmgr.PopScope();
			addInScopeNmsp = false;
			cntNmsp = num;
			break;
		}
		case QilNodeType.AttributeCtor:
			addInScopeNmsp = false;
			CheckNamespaceInScope(nd as QilBinary);
			break;
		case QilNodeType.NamespaceDecl:
			CheckNamespaceInScope(nd as QilBinary);
			break;
		case QilNodeType.Nop:
			AnalyzeContent((nd as QilUnary).Child);
			break;
		default:
			addInScopeNmsp = false;
			break;
		}
	}

	private bool CheckNamespaceInScope(QilBinary nd)
	{
		QilNodeType nodeType = nd.NodeType;
		XPathNodeType nodeKind;
		string text;
		string text2;
		if ((uint)(nodeType - 81) <= 1u)
		{
			QilName qilName = nd.Left as QilName;
			if (!(qilName != null))
			{
				return false;
			}
			text = qilName.Prefix;
			text2 = qilName.NamespaceUri;
			nodeKind = ((nd.NodeType == QilNodeType.ElementCtor) ? XPathNodeType.Element : XPathNodeType.Attribute);
		}
		else
		{
			text = (QilLiteral)nd.Left;
			text2 = (QilLiteral)nd.Right;
			nodeKind = XPathNodeType.Namespace;
		}
		if ((nd.NodeType == QilNodeType.AttributeCtor && text2.Length == 0) || (text == "xml" && text2 == "http://www.w3.org/XML/1998/namespace"))
		{
			XmlILConstructInfo.Write(nd).IsNamespaceInScope = true;
			return true;
		}
		if (!ValidateNames.ValidateName(text, string.Empty, text2, nodeKind, ValidateNames.Flags.CheckPrefixMapping))
		{
			return false;
		}
		text = nsmgr.NameTable.Add(text);
		text2 = nsmgr.NameTable.Add(text2);
		for (int i = 0; i < cntNmsp; i++)
		{
			nsmgr.GetNamespaceDeclaration(i, out var prefix, out var uri);
			if ((object)text == prefix)
			{
				if ((object)text2 == uri)
				{
					XmlILConstructInfo.Write(nd).IsNamespaceInScope = true;
				}
				break;
			}
		}
		if (addInScopeNmsp)
		{
			nsmgr.AddNamespace(text, text2);
			cntNmsp++;
		}
		return true;
	}
}
