using Unity;

namespace System.Text.RegularExpressions;

[Serializable]
public class Capture
{
	internal string _text;

	internal int _index;

	internal int _length;

	public int Index => _index;

	public int Length => _length;

	public string Value => _text.Substring(_index, _length);

	internal Capture(string text, int i, int l)
	{
		_text = text;
		_index = i;
		_length = l;
	}

	public override string ToString()
	{
		return Value;
	}

	internal string GetOriginalString()
	{
		return _text;
	}

	internal string GetLeftSubstring()
	{
		return _text.Substring(0, _index);
	}

	internal string GetRightSubstring()
	{
		return _text.Substring(_index + _length, _text.Length - _index - _length);
	}

	internal Capture()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
