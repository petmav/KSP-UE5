using System.Collections.Generic;
using System.Xml.Xsl.Qil;
using System.Xml.Xsl.XPath;

namespace System.Xml.Xsl.Xslt;

internal struct FunctionFocus : IFocus
{
	private bool isSet;

	private QilParameter current;

	private QilParameter position;

	private QilParameter last;

	public bool IsFocusSet => isSet;

	public void StartFocus(IList<QilNode> args, XslFlags flags)
	{
		int num = 0;
		if ((flags & XslFlags.Current) != 0)
		{
			current = (QilParameter)args[num++];
		}
		if ((flags & XslFlags.Position) != 0)
		{
			position = (QilParameter)args[num++];
		}
		if ((flags & XslFlags.Last) != 0)
		{
			last = (QilParameter)args[num++];
		}
		isSet = true;
	}

	public void StopFocus()
	{
		isSet = false;
		current = (position = (last = null));
	}

	public QilNode GetCurrent()
	{
		return current;
	}

	public QilNode GetPosition()
	{
		return position;
	}

	public QilNode GetLast()
	{
		return last;
	}
}
