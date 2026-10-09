namespace System.CodeDom;

[Serializable]
public class CodeMemberMethod : CodeTypeMember
{
	private readonly CodeParameterDeclarationExpressionCollection _parameters = new CodeParameterDeclarationExpressionCollection();

	private readonly CodeStatementCollection _statements = new CodeStatementCollection();

	private CodeTypeReference _returnType;

	private CodeTypeReferenceCollection _implementationTypes;

	private CodeAttributeDeclarationCollection _returnAttributes;

	private CodeTypeParameterCollection _typeParameters;

	private int _populated;

	private const int ParametersCollection = 1;

	private const int StatementsCollection = 2;

	private const int ImplTypesCollection = 4;

	public CodeTypeReference ReturnType
	{
		get
		{
			return _returnType ?? (_returnType = new CodeTypeReference(typeof(void).FullName));
		}
		set
		{
			_returnType = value;
		}
	}

	public CodeStatementCollection Statements
	{
		get
		{
			if ((_populated & 2) == 0)
			{
				_populated |= 2;
				this.PopulateStatements?.Invoke(this, EventArgs.Empty);
			}
			return _statements;
		}
	}

	public CodeParameterDeclarationExpressionCollection Parameters
	{
		get
		{
			if ((_populated & 1) == 0)
			{
				_populated |= 1;
				this.PopulateParameters?.Invoke(this, EventArgs.Empty);
			}
			return _parameters;
		}
	}

	public CodeTypeReference PrivateImplementationType { get; set; }

	public CodeTypeReferenceCollection ImplementationTypes
	{
		get
		{
			if (_implementationTypes == null)
			{
				_implementationTypes = new CodeTypeReferenceCollection();
			}
			if ((_populated & 4) == 0)
			{
				_populated |= 4;
				this.PopulateImplementationTypes?.Invoke(this, EventArgs.Empty);
			}
			return _implementationTypes;
		}
	}

	public CodeAttributeDeclarationCollection ReturnTypeCustomAttributes => _returnAttributes ?? (_returnAttributes = new CodeAttributeDeclarationCollection());

	public CodeTypeParameterCollection TypeParameters => _typeParameters ?? (_typeParameters = new CodeTypeParameterCollection());

	public event EventHandler PopulateParameters;

	public event EventHandler PopulateStatements;

	public event EventHandler PopulateImplementationTypes;
}
