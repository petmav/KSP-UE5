namespace System.CodeDom;

[Serializable]
public class CodeTypeParameter : CodeObject
{
	private string _name;

	private CodeAttributeDeclarationCollection _customAttributes;

	private CodeTypeReferenceCollection _constraints;

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

	public CodeTypeReferenceCollection Constraints => _constraints ?? (_constraints = new CodeTypeReferenceCollection());

	public CodeAttributeDeclarationCollection CustomAttributes => _customAttributes ?? (_customAttributes = new CodeAttributeDeclarationCollection());

	public bool HasConstructorConstraint { get; set; }

	public CodeTypeParameter()
	{
	}

	public CodeTypeParameter(string name)
	{
		_name = name;
	}
}
