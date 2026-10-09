namespace System.CodeDom;

[Serializable]
public class CodeLabeledStatement : CodeStatement
{
	private string _label;

	public string Label
	{
		get
		{
			return _label ?? string.Empty;
		}
		set
		{
			_label = value;
		}
	}

	public CodeStatement Statement { get; set; }

	public CodeLabeledStatement()
	{
	}

	public CodeLabeledStatement(string label)
	{
		_label = label;
	}

	public CodeLabeledStatement(string label, CodeStatement statement)
	{
		_label = label;
		Statement = statement;
	}
}
