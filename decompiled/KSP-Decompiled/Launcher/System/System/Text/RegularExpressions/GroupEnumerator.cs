using System.Collections;

namespace System.Text.RegularExpressions;

internal class GroupEnumerator : IEnumerator
{
	internal GroupCollection _rgc;

	internal int _curindex;

	public object Current => Capture;

	public Capture Capture
	{
		get
		{
			if (_curindex < 0 || _curindex >= _rgc.Count)
			{
				throw new InvalidOperationException(global::SR.GetString("Enumeration has either not started or has already finished."));
			}
			return _rgc[_curindex];
		}
	}

	internal GroupEnumerator(GroupCollection rgc)
	{
		_curindex = -1;
		_rgc = rgc;
	}

	public bool MoveNext()
	{
		int count = _rgc.Count;
		if (_curindex >= count)
		{
			return false;
		}
		_curindex++;
		return _curindex < count;
	}

	public void Reset()
	{
		_curindex = -1;
	}
}
