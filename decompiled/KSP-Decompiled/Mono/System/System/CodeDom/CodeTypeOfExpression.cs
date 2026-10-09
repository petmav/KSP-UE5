namespace System.CodeDom;

[Serializable]
public class CodeTypeOfExpression : CodeExpression
{
	private CodeTypeReference _type;

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

	public CodeTypeOfExpression()
	{
	}

	public CodeTypeOfExpression(CodeTypeReference type)
	{
		Type = type;
	}

	public CodeTypeOfExpression(string type)
	{
		Type = new CodeTypeReference(type);
	}

	public CodeTypeOfExpression(Type type)
	{
		Type = new CodeTypeReference(type);
	}
}
