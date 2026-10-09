namespace System.Xml.Xsl.Qil;

internal class QilParameter : QilIterator
{
	private QilNode name;

	public override int Count => 2;

	public override QilNode this[int index]
	{
		get
		{
			return index switch
			{
				0 => base.Binding, 
				1 => name, 
				_ => throw new IndexOutOfRangeException(), 
			};
		}
		set
		{
			switch (index)
			{
			case 0:
				base.Binding = value;
				break;
			case 1:
				name = value;
				break;
			default:
				throw new IndexOutOfRangeException();
			}
		}
	}

	public QilNode DefaultValue
	{
		get
		{
			return base.Binding;
		}
		set
		{
			base.Binding = value;
		}
	}

	public QilName Name
	{
		get
		{
			return (QilName)name;
		}
		set
		{
			name = value;
		}
	}

	public QilParameter(QilNodeType nodeType, QilNode defaultValue, QilNode name, XmlQueryType xmlType)
		: base(nodeType, defaultValue)
	{
		this.name = name;
		base.xmlType = xmlType;
	}
}
