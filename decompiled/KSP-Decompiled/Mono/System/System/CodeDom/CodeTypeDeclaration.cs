using System.Reflection;

namespace System.CodeDom;

[Serializable]
public class CodeTypeDeclaration : CodeTypeMember
{
	private readonly CodeTypeReferenceCollection _baseTypes = new CodeTypeReferenceCollection();

	private readonly CodeTypeMemberCollection _members = new CodeTypeMemberCollection();

	private bool _isEnum;

	private bool _isStruct;

	private int _populated;

	private const int BaseTypesCollection = 1;

	private const int MembersCollection = 2;

	private CodeTypeParameterCollection _typeParameters;

	public TypeAttributes TypeAttributes { get; set; } = TypeAttributes.Public;


	public CodeTypeReferenceCollection BaseTypes
	{
		get
		{
			if ((_populated & 1) == 0)
			{
				_populated |= 1;
				this.PopulateBaseTypes?.Invoke(this, EventArgs.Empty);
			}
			return _baseTypes;
		}
	}

	public bool IsClass
	{
		get
		{
			if ((TypeAttributes & TypeAttributes.ClassSemanticsMask) == 0 && !_isEnum)
			{
				return !_isStruct;
			}
			return false;
		}
		set
		{
			if (value)
			{
				TypeAttributes &= ~TypeAttributes.ClassSemanticsMask;
				TypeAttributes |= TypeAttributes.NotPublic;
				_isStruct = false;
				_isEnum = false;
			}
		}
	}

	public bool IsStruct
	{
		get
		{
			return _isStruct;
		}
		set
		{
			if (value)
			{
				TypeAttributes &= ~TypeAttributes.ClassSemanticsMask;
				_isEnum = false;
			}
			_isStruct = value;
		}
	}

	public bool IsEnum
	{
		get
		{
			return _isEnum;
		}
		set
		{
			if (value)
			{
				TypeAttributes &= ~TypeAttributes.ClassSemanticsMask;
				_isStruct = false;
			}
			_isEnum = value;
		}
	}

	public bool IsInterface
	{
		get
		{
			return (TypeAttributes & TypeAttributes.ClassSemanticsMask) == TypeAttributes.ClassSemanticsMask;
		}
		set
		{
			if (value)
			{
				TypeAttributes &= ~TypeAttributes.ClassSemanticsMask;
				TypeAttributes |= TypeAttributes.ClassSemanticsMask;
				_isStruct = false;
				_isEnum = false;
			}
			else
			{
				TypeAttributes &= ~TypeAttributes.ClassSemanticsMask;
			}
		}
	}

	public bool IsPartial { get; set; }

	public CodeTypeMemberCollection Members
	{
		get
		{
			if ((_populated & 2) == 0)
			{
				_populated |= 2;
				this.PopulateMembers?.Invoke(this, EventArgs.Empty);
			}
			return _members;
		}
	}

	public CodeTypeParameterCollection TypeParameters => _typeParameters ?? (_typeParameters = new CodeTypeParameterCollection());

	public event EventHandler PopulateBaseTypes;

	public event EventHandler PopulateMembers;

	public CodeTypeDeclaration()
	{
	}

	public CodeTypeDeclaration(string name)
	{
		base.Name = name;
	}
}
