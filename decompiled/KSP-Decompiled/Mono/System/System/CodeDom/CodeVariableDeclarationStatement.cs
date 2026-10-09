namespace System.CodeDom;

[Serializable]
public class CodeVariableDeclarationStatement : CodeStatement
{
	private CodeTypeReference _type;

	private string _name;

	public CodeExpression InitExpression { get; set; }

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

	public CodeTypeReference Type
	{
		get
		{
			return _type ?? (_type = new CodeTypeReference(""));
		}
		set
		{
			_type = value;
		}
	}

	public CodeVariableDeclarationStatement()
	{
	}

	public CodeVariableDeclarationStatement(CodeTypeReference type, string name)
	{
		Type = type;
		Name = name;
	}

	public CodeVariableDeclarationStatement(string type, string name)
	{
		Type = new CodeTypeReference(type);
		Name = name;
	}

	public CodeVariableDeclarationStatement(Type type, string name)
	{
		Type = new CodeTypeReference(type);
		Name = name;
	}

	public CodeVariableDeclarationStatement(CodeTypeReference type, string name, CodeExpression initExpression)
	{
		Type = type;
		Name = name;
		InitExpression = initExpression;
	}

	public CodeVariableDeclarationStatement(string type, string name, CodeExpression initExpression)
	{
		Type = new CodeTypeReference(type);
		Name = name;
		InitExpression = initExpression;
	}

	public CodeVariableDeclarationStatement(Type type, string name, CodeExpression initExpression)
	{
		Type = new CodeTypeReference(type);
		Name = name;
		InitExpression = initExpression;
	}
}
