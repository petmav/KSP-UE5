namespace System.Xml.Xsl.Qil;

internal class QilUnary : QilNode
{
	private QilNode child;

	public override int Count => 1;

	public override QilNode this[int index]
	{
		get
		{
			if (index != 0)
			{
				throw new IndexOutOfRangeException();
			}
			return child;
		}
		set
		{
			if (index != 0)
			{
				throw new IndexOutOfRangeException();
			}
			child = value;
		}
	}

	public QilNode Child
	{
		get
		{
			return child;
		}
		set
		{
			child = value;
		}
	}

	public QilUnary(QilNodeType nodeType, QilNode child)
		: base(nodeType)
	{
		this.child = child;
	}
}
