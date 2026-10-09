using System.Collections.Generic;
using System.Xml.Xsl.Qil;

namespace System.Xml.Xsl.Xslt;

internal class TemplateMatch
{
	internal class TemplateMatchComparer : IComparer<TemplateMatch>
	{
		public int Compare(TemplateMatch x, TemplateMatch y)
		{
			if (!(x.priority > y.priority))
			{
				if (!(x.priority < y.priority))
				{
					return x.template.OrderNumber - y.template.OrderNumber;
				}
				return -1;
			}
			return 1;
		}
	}

	public static readonly TemplateMatchComparer Comparer = new TemplateMatchComparer();

	private Template template;

	private double priority;

	private XmlNodeKindFlags nodeKind;

	private QilName qname;

	private QilIterator iterator;

	private QilNode condition;

	public XmlNodeKindFlags NodeKind => nodeKind;

	public QilName QName => qname;

	public QilIterator Iterator => iterator;

	public QilNode Condition => condition;

	public QilFunction TemplateFunction => template.Function;

	public TemplateMatch(Template template, QilLoop filter)
	{
		this.template = template;
		priority = (double.IsNaN(template.Priority) ? XPathPatternBuilder.GetPriority(filter) : template.Priority);
		iterator = filter.Variable;
		condition = filter.Body;
		XPathPatternBuilder.CleanAnnotation(filter);
		NipOffTypeNameCheck();
	}

	private void NipOffTypeNameCheck()
	{
		QilBinary[] array = new QilBinary[4];
		int num = -1;
		QilNode left = condition;
		nodeKind = XmlNodeKindFlags.None;
		qname = null;
		while (left.NodeType == QilNodeType.And)
		{
			left = (array[++num & 3] = (QilBinary)left).Left;
		}
		if (left.NodeType != QilNodeType.IsType)
		{
			return;
		}
		QilBinary qilBinary = (QilBinary)left;
		if (qilBinary.Left != iterator || qilBinary.Right.NodeType != QilNodeType.LiteralType)
		{
			return;
		}
		XmlNodeKindFlags nodeKinds = qilBinary.Right.XmlType.NodeKinds;
		if (!Bits.ExactlyOne((uint)nodeKinds))
		{
			return;
		}
		nodeKind = nodeKinds;
		QilBinary qilBinary2 = array[num & 3];
		if (qilBinary2 != null && qilBinary2.Right.NodeType == QilNodeType.Eq)
		{
			QilBinary qilBinary3 = (QilBinary)qilBinary2.Right;
			if (qilBinary3.Left.NodeType == QilNodeType.NameOf && ((QilUnary)qilBinary3.Left).Child == iterator && qilBinary3.Right.NodeType == QilNodeType.LiteralQName)
			{
				qname = (QilName)((QilLiteral)qilBinary3.Right).Value;
				num--;
			}
		}
		QilBinary qilBinary4 = array[num & 3];
		QilBinary qilBinary5 = array[--num & 3];
		if (qilBinary5 != null)
		{
			qilBinary5.Left = qilBinary4.Right;
		}
		else if (qilBinary4 != null)
		{
			condition = qilBinary4.Right;
		}
		else
		{
			condition = null;
		}
	}
}
