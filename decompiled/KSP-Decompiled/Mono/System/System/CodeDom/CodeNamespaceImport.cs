namespace System.CodeDom;

[Serializable]
public class CodeNamespaceImport : CodeObject
{
	private string _nameSpace;

	public CodeLinePragma LinePragma { get; set; }

	public string Namespace
	{
		get
		{
			return _nameSpace ?? string.Empty;
		}
		set
		{
			_nameSpace = value;
		}
	}

	public CodeNamespaceImport()
	{
	}

	public CodeNamespaceImport(string nameSpace)
	{
		Namespace = nameSpace;
	}
}
