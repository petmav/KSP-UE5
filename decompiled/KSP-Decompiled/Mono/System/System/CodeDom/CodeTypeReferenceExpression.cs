namespace System.CodeDom;

[Serializable]
public class CodeTypeReferenceExpression : CodeExpression
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

	public CodeTypeReferenceExpression()
	{
	}

	public CodeTypeReferenceExpression(CodeTypeReference type)
	{
		Type = type;
	}

	public CodeTypeReferenceExpression(string type)
	{
		Type = new CodeTypeReference(type);
	}

	public CodeTypeReferenceExpression(Type type)
	{
		Type = new CodeTypeReference(type);
	}
}
