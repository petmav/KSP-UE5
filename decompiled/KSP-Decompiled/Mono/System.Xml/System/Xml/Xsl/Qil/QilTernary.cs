namespace System.Xml.Xsl.Qil;

internal class QilTernary : QilNode
{
	private QilNode left;

	private QilNode center;

	private QilNode right;

	public override int Count => 3;

	public override QilNode this[int index]
	{
		get
		{
			return index switch
			{
				0 => left, 
				1 => center, 
				2 => right, 
				_ => throw new IndexOutOfRangeException(), 
			};
		}
		set
		{
			switch (index)
			{
			case 0:
				left = value;
				break;
			case 1:
				center = value;
				break;
			case 2:
				right = value;
				break;
			default:
				throw new IndexOutOfRangeException();
			}
		}
	}

	public QilNode Left
	{
		get
		{
			return left;
		}
		set
		{
			left = value;
		}
	}

	public QilNode Center
	{
		get
		{
			return center;
		}
		set
		{
			center = value;
		}
	}

	public QilNode Right
	{
		get
		{
			return right;
		}
		set
		{
			right = value;
		}
	}

	public QilTernary(QilNodeType nodeType, QilNode left, QilNode center, QilNode right)
		: base(nodeType)
	{
		this.left = left;
		this.center = center;
		this.right = right;
	}
}
