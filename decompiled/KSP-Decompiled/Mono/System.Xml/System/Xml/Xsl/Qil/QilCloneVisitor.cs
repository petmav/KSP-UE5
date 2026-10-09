namespace System.Xml.Xsl.Qil;

internal class QilCloneVisitor : QilScopedVisitor
{
	private QilFactory fac;

	private SubstitutionList subs;

	public QilCloneVisitor(QilFactory fac)
		: this(fac, new SubstitutionList())
	{
	}

	public QilCloneVisitor(QilFactory fac, SubstitutionList subs)
	{
		this.fac = fac;
		this.subs = subs;
	}

	public QilNode Clone(QilNode node)
	{
		QilDepthChecker.Check(node);
		return VisitAssumeReference(node);
	}

	protected override QilNode Visit(QilNode oldNode)
	{
		QilNode qilNode = null;
		if (oldNode == null)
		{
			return null;
		}
		if (oldNode is QilReference)
		{
			qilNode = FindClonedReference(oldNode);
		}
		if (qilNode == null)
		{
			qilNode = oldNode.ShallowClone(fac);
		}
		return base.Visit(qilNode);
	}

	protected override QilNode VisitChildren(QilNode parent)
	{
		for (int i = 0; i < parent.Count; i++)
		{
			QilNode qilNode = parent[i];
			if (IsReference(parent, i))
			{
				parent[i] = VisitReference(qilNode);
				if (parent[i] == null)
				{
					parent[i] = qilNode;
				}
			}
			else
			{
				parent[i] = Visit(qilNode);
			}
		}
		return parent;
	}

	protected override QilNode VisitReference(QilNode oldNode)
	{
		QilNode qilNode = FindClonedReference(oldNode);
		return base.VisitReference((qilNode == null) ? oldNode : qilNode);
	}

	protected override void BeginScope(QilNode node)
	{
		subs.AddSubstitutionPair(node, node.ShallowClone(fac));
	}

	protected override void EndScope(QilNode node)
	{
		subs.RemoveLastSubstitutionPair();
	}

	protected QilNode FindClonedReference(QilNode node)
	{
		return subs.FindReplacement(node);
	}
}
