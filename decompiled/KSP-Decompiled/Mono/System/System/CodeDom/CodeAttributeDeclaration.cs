namespace System.CodeDom;

[Serializable]
public class CodeAttributeDeclaration
{
	private string _name;

	private readonly CodeAttributeArgumentCollection _arguments = new CodeAttributeArgumentCollection();

	private CodeTypeReference _attributeType;

	public string Name
	{
		get
		{
			return _name ?? string.Empty;
		}
		set
		{
			_name = value;
			_attributeType = new CodeTypeReference(_name);
		}
	}

	public CodeAttributeArgumentCollection Arguments => _arguments;

	public CodeTypeReference AttributeType => _attributeType;

	public CodeAttributeDeclaration()
	{
	}

	public CodeAttributeDeclaration(string name)
	{
		Name = name;
	}

	public CodeAttributeDeclaration(string name, params CodeAttributeArgument[] arguments)
	{
		Name = name;
		Arguments.AddRange(arguments);
	}

	public CodeAttributeDeclaration(CodeTypeReference attributeType)
		: this(attributeType, (CodeAttributeArgument[])null)
	{
	}

	public CodeAttributeDeclaration(CodeTypeReference attributeType, params CodeAttributeArgument[] arguments)
	{
		_attributeType = attributeType;
		if (attributeType != null)
		{
			_name = attributeType.BaseType;
		}
		if (arguments != null)
		{
			Arguments.AddRange(arguments);
		}
	}
}
