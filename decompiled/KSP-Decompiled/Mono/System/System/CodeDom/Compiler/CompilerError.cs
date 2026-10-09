using System.Globalization;

namespace System.CodeDom.Compiler;

[Serializable]
public class CompilerError
{
	public int Line { get; set; }

	public int Column { get; set; }

	public string ErrorNumber { get; set; }

	public string ErrorText { get; set; }

	public bool IsWarning { get; set; }

	public string FileName { get; set; }

	private string WarningString
	{
		get
		{
			if (!IsWarning)
			{
				return "error";
			}
			return "warning";
		}
	}

	public CompilerError()
		: this(string.Empty, 0, 0, string.Empty, string.Empty)
	{
	}

	public CompilerError(string fileName, int line, int column, string errorNumber, string errorText)
	{
		Line = line;
		Column = column;
		ErrorNumber = errorNumber;
		ErrorText = errorText;
		FileName = fileName;
	}

	public override string ToString()
	{
		if (FileName.Length <= 0)
		{
			return string.Format(CultureInfo.InvariantCulture, "{0} {1}: {2}", WarningString, ErrorNumber, ErrorText);
		}
		return string.Format(CultureInfo.InvariantCulture, "{0}({1},{2}) : {3} {4}: {5}", FileName, Line, Column, WarningString, ErrorNumber, ErrorText);
	}
}
