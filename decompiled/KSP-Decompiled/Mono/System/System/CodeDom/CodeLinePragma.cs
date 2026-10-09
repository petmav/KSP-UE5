namespace System.CodeDom;

[Serializable]
public class CodeLinePragma
{
	private string _fileName;

	public string FileName
	{
		get
		{
			return _fileName ?? string.Empty;
		}
		set
		{
			_fileName = value;
		}
	}

	public int LineNumber { get; set; }

	public CodeLinePragma()
	{
	}

	public CodeLinePragma(string fileName, int lineNumber)
	{
		FileName = fileName;
		LineNumber = lineNumber;
	}
}
