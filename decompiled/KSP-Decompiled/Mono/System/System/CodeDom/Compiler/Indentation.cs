namespace System.CodeDom.Compiler;

internal sealed class Indentation
{
	private readonly ExposedTabStringIndentedTextWriter _writer;

	private readonly int _indent;

	private string _s;

	internal string IndentationString
	{
		get
		{
			if (_s == null)
			{
				string tabString = _writer.TabString;
				switch (_indent)
				{
				case 0:
					_s = string.Empty;
					break;
				case 1:
					_s = tabString;
					break;
				case 2:
					_s = tabString + tabString;
					break;
				case 3:
					_s = tabString + tabString + tabString;
					break;
				case 4:
					_s = tabString + tabString + tabString + tabString;
					break;
				default:
				{
					string[] array = new string[_indent];
					for (int i = 0; i < array.Length; i++)
					{
						array[i] = tabString;
					}
					return string.Concat(array);
				}
				}
			}
			return _s;
		}
	}

	internal Indentation(ExposedTabStringIndentedTextWriter writer, int indent)
	{
		_writer = writer;
		_indent = indent;
	}
}
