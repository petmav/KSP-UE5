namespace System.Xml.Xsl.Qil;

internal class QilFunction : QilReference
{
	private QilNode arguments;

	private QilNode definition;

	private QilNode sideEffects;

	public override int Count => 3;

	public override QilNode this[int index]
	{
		get
		{
			return index switch
			{
				0 => arguments, 
				1 => definition, 
				2 => sideEffects, 
				_ => throw new IndexOutOfRangeException(), 
			};
		}
		set
		{
			switch (index)
			{
			case 0:
				arguments = value;
				break;
			case 1:
				definition = value;
				break;
			case 2:
				sideEffects = value;
				break;
			default:
				throw new IndexOutOfRangeException();
			}
		}
	}

	public QilList Arguments
	{
		get
		{
			return (QilList)arguments;
		}
		set
		{
			arguments = value;
		}
	}

	public QilNode Definition
	{
		get
		{
			return definition;
		}
		set
		{
			definition = value;
		}
	}

	public bool MaybeSideEffects
	{
		get
		{
			return sideEffects.NodeType == QilNodeType.True;
		}
		set
		{
			sideEffects.NodeType = (value ? QilNodeType.True : QilNodeType.False);
		}
	}

	public QilFunction(QilNodeType nodeType, QilNode arguments, QilNode definition, QilNode sideEffects, XmlQueryType resultType)
		: base(nodeType)
	{
		this.arguments = arguments;
		this.definition = definition;
		this.sideEffects = sideEffects;
		xmlType = resultType;
	}
}
