using System.Collections.Generic;
using System.Globalization;
using System.Text;

namespace System.Xml.Xsl.Qil;

internal class QilXmlWriter : QilScopedVisitor
{
	[Flags]
	public enum Options
	{
		None = 0,
		Annotations = 1,
		TypeInfo = 2,
		RoundTripTypeInfo = 4,
		LineInfo = 8,
		NodeIdentity = 0x10,
		NodeLocation = 0x20
	}

	internal class ForwardRefFinder : QilVisitor
	{
		private List<QilNode> fwdrefs = new List<QilNode>();

		private List<QilNode> backrefs = new List<QilNode>();

		public IList<QilNode> Find(QilExpression qil)
		{
			Visit(qil);
			return fwdrefs;
		}

		protected override QilNode Visit(QilNode node)
		{
			if (node is QilIterator || node is QilFunction)
			{
				backrefs.Add(node);
			}
			return base.Visit(node);
		}

		protected override QilNode VisitReference(QilNode node)
		{
			if (!backrefs.Contains(node) && !fwdrefs.Contains(node))
			{
				fwdrefs.Add(node);
			}
			return node;
		}
	}

	private sealed class NameGenerator
	{
		private class NameAnnotation : ListBase<object>
		{
			public string Name;

			public object PriorAnnotation;

			public override int Count => 1;

			public override object this[int index]
			{
				get
				{
					if (index == 0)
					{
						return PriorAnnotation;
					}
					throw new IndexOutOfRangeException();
				}
				set
				{
					throw new NotSupportedException();
				}
			}

			public NameAnnotation(string s, object a)
			{
				Name = s;
				PriorAnnotation = a;
			}
		}

		private StringBuilder name;

		private int len;

		private int zero;

		private char start;

		private char end;

		public NameGenerator()
		{
			string text = "$";
			len = (zero = text.Length);
			start = 'a';
			end = 'z';
			name = new StringBuilder(text, len + 2);
			name.Append(start);
		}

		public string NextName()
		{
			string result = name.ToString();
			char c = name[len];
			if (c != end)
			{
				c = (name[len] = (char)(c + 1));
			}
			else
			{
				name[len] = start;
				int num = len;
				while (num-- > zero && name[num] == end)
				{
					name[num] = start;
				}
				if (num < zero)
				{
					len++;
					name.Append(start);
				}
				else
				{
					name[num]++;
				}
			}
			return result;
		}

		public string NameOf(QilNode n)
		{
			string text = null;
			object annotation = n.Annotation;
			if (!(annotation is NameAnnotation nameAnnotation))
			{
				text = NextName();
				n.Annotation = new NameAnnotation(text, annotation);
			}
			else
			{
				text = nameAnnotation.Name;
			}
			return text;
		}

		public void ClearName(QilNode n)
		{
			if (n.Annotation is NameAnnotation)
			{
				n.Annotation = ((NameAnnotation)n.Annotation).PriorAnnotation;
			}
		}
	}

	protected XmlWriter writer;

	protected Options options;

	private NameGenerator ngen;

	public QilXmlWriter(XmlWriter writer)
		: this(writer, Options.Annotations | Options.TypeInfo | Options.LineInfo | Options.NodeIdentity | Options.NodeLocation)
	{
	}

	public QilXmlWriter(XmlWriter writer, Options options)
	{
		this.writer = writer;
		ngen = new NameGenerator();
		this.options = options;
	}

	public void ToXml(QilNode node)
	{
		VisitAssumeReference(node);
	}

	protected virtual void WriteAnnotations(object ann)
	{
		string text = null;
		string text2 = null;
		if (ann == null)
		{
			return;
		}
		if (ann is string)
		{
			text = ann as string;
		}
		else if (ann is IQilAnnotation)
		{
			text2 = (ann as IQilAnnotation).Name;
			text = ann.ToString();
		}
		else if (ann is IList<object>)
		{
			foreach (object item in (IList<object>)ann)
			{
				WriteAnnotations(item);
			}
			return;
		}
		if (text != null && text.Length != 0)
		{
			writer.WriteComment((text2 != null && text2.Length != 0) ? (text2 + ": " + text) : text);
		}
	}

	protected virtual void WriteLineInfo(QilNode node)
	{
		writer.WriteAttributeString("lineInfo", string.Format(CultureInfo.InvariantCulture, "[{0},{1} -- {2},{3}]", node.SourceLine.Start.Line, node.SourceLine.Start.Pos, node.SourceLine.End.Line, node.SourceLine.End.Pos));
	}

	protected virtual void WriteXmlType(QilNode node)
	{
		writer.WriteAttributeString("xmlType", node.XmlType.ToString(((options & Options.RoundTripTypeInfo) != 0) ? "S" : "G"));
	}

	protected override QilNode VisitChildren(QilNode node)
	{
		if (node is QilLiteral)
		{
			writer.WriteValue(Convert.ToString(((QilLiteral)node).Value, CultureInfo.InvariantCulture));
			return node;
		}
		if (node is QilReference)
		{
			QilReference qilReference = (QilReference)node;
			writer.WriteAttributeString("id", ngen.NameOf(node));
			if (qilReference.DebugName != null)
			{
				writer.WriteAttributeString("name", qilReference.DebugName.ToString());
			}
			if (node.NodeType == QilNodeType.Parameter)
			{
				QilParameter qilParameter = (QilParameter)node;
				if (qilParameter.DefaultValue != null)
				{
					VisitAssumeReference(qilParameter.DefaultValue);
				}
				return node;
			}
		}
		return base.VisitChildren(node);
	}

	protected override QilNode VisitReference(QilNode node)
	{
		QilReference qilReference = (QilReference)node;
		string text = ngen.NameOf(node);
		if (text == null)
		{
			text = "OUT-OF-SCOPE REFERENCE";
		}
		writer.WriteStartElement("RefTo");
		writer.WriteAttributeString("id", text);
		if (qilReference.DebugName != null)
		{
			writer.WriteAttributeString("name", qilReference.DebugName.ToString());
		}
		writer.WriteEndElement();
		return node;
	}

	protected override QilNode VisitQilExpression(QilExpression qil)
	{
		IList<QilNode> list = new ForwardRefFinder().Find(qil);
		if (list != null && list.Count > 0)
		{
			writer.WriteStartElement("ForwardDecls");
			foreach (QilNode item in list)
			{
				writer.WriteStartElement(Enum.GetName(typeof(QilNodeType), item.NodeType));
				writer.WriteAttributeString("id", ngen.NameOf(item));
				WriteXmlType(item);
				if (item.NodeType == QilNodeType.Function)
				{
					Visit(item[0]);
					Visit(item[2]);
				}
				writer.WriteEndElement();
			}
			writer.WriteEndElement();
		}
		return VisitChildren(qil);
	}

	protected override QilNode VisitLiteralType(QilLiteral value)
	{
		writer.WriteString(((XmlQueryType)value).ToString(((options & Options.TypeInfo) != 0) ? "G" : "S"));
		return value;
	}

	protected override QilNode VisitLiteralQName(QilName value)
	{
		writer.WriteAttributeString("name", value.ToString());
		return value;
	}

	protected override void BeginScope(QilNode node)
	{
		ngen.NameOf(node);
	}

	protected override void EndScope(QilNode node)
	{
		ngen.ClearName(node);
	}

	protected override void BeforeVisit(QilNode node)
	{
		base.BeforeVisit(node);
		if ((options & Options.Annotations) != 0)
		{
			WriteAnnotations(node.Annotation);
		}
		writer.WriteStartElement("", Enum.GetName(typeof(QilNodeType), node.NodeType), "");
		if ((options & (Options.TypeInfo | Options.RoundTripTypeInfo)) != 0)
		{
			WriteXmlType(node);
		}
		if ((options & Options.LineInfo) != 0 && node.SourceLine != null)
		{
			WriteLineInfo(node);
		}
	}

	protected override void AfterVisit(QilNode node)
	{
		writer.WriteEndElement();
		base.AfterVisit(node);
	}
}
