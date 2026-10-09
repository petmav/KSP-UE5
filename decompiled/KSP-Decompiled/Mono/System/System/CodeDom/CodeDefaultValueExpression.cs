namespace System.CodeDom;

[Serializable]
public class CodeDefaultValueExpression : CodeExpression
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

	public CodeDefaultValueExpression()
	{
	}

	public CodeDefaultValueExpression(CodeTypeReference type)
	{
		_type = type;
	}
}
