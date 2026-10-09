using System.Collections;
using System.Xml.Xsl.Qil;

namespace System.Xml.Xsl.IlGen;

internal class XmlILElementAnalyzer : XmlILStateAnalyzer
{
	private NameTable attrNames = new NameTable();

	private ArrayList dupAttrs = new ArrayList();

	public XmlILElementAnalyzer(QilFactory fac)
		: base(fac)
	{
	}

	public override QilNode Analyze(QilNode ndElem, QilNode ndContent)
	{
		parentInfo = XmlILConstructInfo.Write(ndElem);
		parentInfo.MightHaveNamespacesAfterAttributes = false;
		parentInfo.MightHaveAttributes = false;
		parentInfo.MightHaveDuplicateAttributes = false;
		parentInfo.MightHaveNamespaces = !parentInfo.IsNamespaceInScope;
		dupAttrs.Clear();
		return base.Analyze(ndElem, ndContent);
	}

	protected override void AnalyzeLoop(QilLoop ndLoop, XmlILConstructInfo info)
	{
		if (ndLoop.XmlType.MaybeMany)
		{
			CheckAttributeNamespaceConstruct(ndLoop.XmlType);
		}
		base.AnalyzeLoop(ndLoop, info);
	}

	protected override void AnalyzeCopy(QilNode ndCopy, XmlILConstructInfo info)
	{
		if (ndCopy.NodeType == QilNodeType.AttributeCtor)
		{
			AnalyzeAttributeCtor(ndCopy as QilBinary, info);
		}
		else
		{
			CheckAttributeNamespaceConstruct(ndCopy.XmlType);
		}
		base.AnalyzeCopy(ndCopy, info);
	}

	private void AnalyzeAttributeCtor(QilBinary ndAttr, XmlILConstructInfo info)
	{
		if (ndAttr.Left.NodeType == QilNodeType.LiteralQName)
		{
			QilName qilName = ndAttr.Left as QilName;
			parentInfo.MightHaveAttributes = true;
			if (!parentInfo.MightHaveDuplicateAttributes)
			{
				XmlQualifiedName xmlQualifiedName = new XmlQualifiedName(attrNames.Add(qilName.LocalName), attrNames.Add(qilName.NamespaceUri));
				int i;
				for (i = 0; i < dupAttrs.Count; i++)
				{
					XmlQualifiedName xmlQualifiedName2 = (XmlQualifiedName)dupAttrs[i];
					if ((object)xmlQualifiedName2.Name == xmlQualifiedName.Name && (object)xmlQualifiedName2.Namespace == xmlQualifiedName.Namespace)
					{
						parentInfo.MightHaveDuplicateAttributes = true;
					}
				}
				if (i >= dupAttrs.Count)
				{
					dupAttrs.Add(xmlQualifiedName);
				}
			}
			if (!info.IsNamespaceInScope)
			{
				parentInfo.MightHaveNamespaces = true;
			}
		}
		else
		{
			CheckAttributeNamespaceConstruct(ndAttr.XmlType);
		}
	}

	private void CheckAttributeNamespaceConstruct(XmlQueryType typ)
	{
		if ((typ.NodeKinds & XmlNodeKindFlags.Attribute) != 0)
		{
			parentInfo.MightHaveAttributes = true;
			parentInfo.MightHaveDuplicateAttributes = true;
			parentInfo.MightHaveNamespaces = true;
		}
		if ((typ.NodeKinds & XmlNodeKindFlags.Namespace) != 0)
		{
			parentInfo.MightHaveNamespaces = true;
			if (parentInfo.MightHaveAttributes)
			{
				parentInfo.MightHaveNamespacesAfterAttributes = true;
			}
		}
	}
}
