namespace System.Xml.Xsl.Qil;

internal class QilReference : QilNode
{
	private const int MaxDebugNameLength = 1000;

	private string debugName;

	public string DebugName
	{
		get
		{
			return debugName;
		}
		set
		{
			if (value.Length > 1000)
			{
				value = value.Substring(0, 1000);
			}
			debugName = value;
		}
	}

	public QilReference(QilNodeType nodeType)
		: base(nodeType)
	{
	}
}
