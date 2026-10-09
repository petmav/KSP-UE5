namespace System.CodeDom;

[Serializable]
public class CodeMemberField : CodeTypeMember
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

	public CodeExpression InitExpression { get; set; }

	public CodeMemberField()
	{
	}

	public CodeMemberField(CodeTypeReference type, string name)
	{
		Type = type;
		base.Name = name;
	}

	public CodeMemberField(string type, string name)
	{
		Type = new CodeTypeReference(type);
		base.Name = name;
	}

	public CodeMemberField(Type type, string name)
	{
		Type = new CodeTypeReference(type);
		base.Name = name;
	}
}
