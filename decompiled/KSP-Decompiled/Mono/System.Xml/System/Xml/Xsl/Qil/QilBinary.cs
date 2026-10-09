namespace System.Xml.Xsl.Qil;

internal class QilBinary : QilNode
{
	private QilNode left;

	private QilNode right;

	public override int Count => 2;

	public override QilNode this[int index]
	{
		get
		{
			return index switch
			{
				0 => left, 
				1 => right, 
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

	public QilBinary(QilNodeType nodeType, QilNode left, QilNode right)
		: base(nodeType)
	{
		this.left = left;
		this.right = right;
	}
}
