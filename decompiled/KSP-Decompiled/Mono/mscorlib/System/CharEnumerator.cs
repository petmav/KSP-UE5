using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using Unity;

namespace System;

[Serializable]
[ComVisible(true)]
public sealed class CharEnumerator : IEnumerator, ICloneable, IEnumerator<char>, IDisposable
{
	private string str;

	private int index;

	private char currentElement;

	object IEnumerator.Current
	{
		get
		{
			if (index == -1)
			{
				throw new InvalidOperationException(Environment.GetResourceString("Enumeration has not started. Call MoveNext."));
			}
			if (index >= str.Length)
			{
				throw new InvalidOperationException(Environment.GetResourceString("Enumeration already finished."));
			}
			return currentElement;
		}
	}

	public char Current
	{
		get
		{
			if (index == -1)
			{
				throw new InvalidOperationException(Environment.GetResourceString("Enumeration has not started. Call MoveNext."));
			}
			if (index >= str.Length)
			{
				throw new InvalidOperationException(Environment.GetResourceString("Enumeration already finished."));
			}
			return currentElement;
		}
	}

	internal CharEnumerator(string str)
	{
		this.str = str;
		index = -1;
	}

	public object Clone()
	{
		return MemberwiseClone();
	}

	public bool MoveNext()
	{
		if (index < str.Length - 1)
		{
			index++;
			currentElement = str[index];
			return true;
		}
		index = str.Length;
		return false;
	}

	public void Dispose()
	{
		if (str != null)
		{
			index = str.Length;
		}
		str = null;
	}

	public void Reset()
	{
		currentElement = '\0';
		index = -1;
	}

	internal CharEnumerator()
	{
		ThrowStub.ThrowNotSupportedException();
	}
}
