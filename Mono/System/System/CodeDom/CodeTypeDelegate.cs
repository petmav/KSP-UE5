using System.Reflection;

namespace System.CodeDom;

[Serializable]
public class CodeTypeDelegate : CodeTypeDeclaration
{
	private CodeTypeReference _returnType;

	public CodeTypeReference ReturnType
	{
		get
		{
			return _returnType ?? (_returnType = new CodeTypeReference(""));
		}
		set
		{
			_returnType = value;
		}
	}

	public CodeParameterDeclarationExpressionCollection Parameters { get; } = new CodeParameterDeclarationExpressionCollection();


	public CodeTypeDelegate()
	{
		base.TypeAttributes &= ~TypeAttributes.ClassSemanticsMask;
		base.TypeAttributes |= TypeAttributes.NotPublic;
		base.BaseTypes.Clear();
		base.BaseTypes.Add(new CodeTypeReference("System.Delegate"));
	}

	public CodeTypeDelegate(string name)
		: this()
	{
		base.Name = name;
	}
}
