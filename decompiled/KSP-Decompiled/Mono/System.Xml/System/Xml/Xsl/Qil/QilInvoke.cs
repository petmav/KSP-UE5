namespace System.Xml.Xsl.Qil;

internal class QilInvoke : QilBinary
{
	public QilFunction Function
	{
		get
		{
			return (QilFunction)base.Left;
		}
		set
		{
			base.Left = value;
		}
	}

	public QilList Arguments
	{
		get
		{
			return (QilList)base.Right;
		}
		set
		{
			base.Right = value;
		}
	}

	public QilInvoke(QilNodeType nodeType, QilNode function, QilNode arguments)
		: base(nodeType, function, arguments)
	{
	}
}
