namespace System.CodeDom;

[Serializable]
public class CodeAttributeArgument
{
	private string _name;

	public string Name
	{
		get
		{
			return _name ?? string.Empty;
		}
		set
		{
			_name = value;
		}
	}

	public CodeExpression Value { get; set; }

	public CodeAttributeArgument()
	{
	}

	public CodeAttributeArgument(CodeExpression value)
	{
		Value = value;
	}

	public CodeAttributeArgument(string name, CodeExpression value)
	{
		Name = name;
		Value = value;
	}
}
