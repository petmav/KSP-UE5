namespace System.CodeDom;

[Serializable]
public class CodeObjectCreateExpression : CodeExpression
{
	private CodeTypeReference _createType;

	public CodeTypeReference CreateType
	{
		get
		{
			return _createType ?? (_createType = new CodeTypeReference(""));
		}
		set
		{
			_createType = value;
		}
	}

	public CodeExpressionCollection Parameters { get; } = new CodeExpressionCollection();


	public CodeObjectCreateExpression()
	{
	}

	public CodeObjectCreateExpression(CodeTypeReference createType, params CodeExpression[] parameters)
	{
		CreateType = createType;
		Parameters.AddRange(parameters);
	}

	public CodeObjectCreateExpression(string createType, params CodeExpression[] parameters)
	{
		CreateType = new CodeTypeReference(createType);
		Parameters.AddRange(parameters);
	}

	public CodeObjectCreateExpression(Type createType, params CodeExpression[] parameters)
	{
		CreateType = new CodeTypeReference(createType);
		Parameters.AddRange(parameters);
	}
}
