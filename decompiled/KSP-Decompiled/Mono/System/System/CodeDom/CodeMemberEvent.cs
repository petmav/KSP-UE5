namespace System.CodeDom;

[Serializable]
public class CodeMemberEvent : CodeTypeMember
{
	private CodeTypeReference _type;

	private CodeTypeReferenceCollection _implementationTypes;

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

	public CodeTypeReference PrivateImplementationType { get; set; }

	public CodeTypeReferenceCollection ImplementationTypes => _implementationTypes ?? (_implementationTypes = new CodeTypeReferenceCollection());
}
