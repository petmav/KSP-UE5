namespace System.Xml.Xsl.Qil;

internal class QilIterator : QilReference
{
	private QilNode binding;

	public override int Count => 1;

	public override QilNode this[int index]
	{
		get
		{
			if (index != 0)
			{
				throw new IndexOutOfRangeException();
			}
			return binding;
		}
		set
		{
			if (index != 0)
			{
				throw new IndexOutOfRangeException();
			}
			binding = value;
		}
	}

	public QilNode Binding
	{
		get
		{
			return binding;
		}
		set
		{
			binding = value;
		}
	}

	public QilIterator(QilNodeType nodeType, QilNode binding)
		: base(nodeType)
	{
		Binding = binding;
	}
}
