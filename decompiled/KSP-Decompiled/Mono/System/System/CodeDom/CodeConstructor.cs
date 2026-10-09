namespace System.CodeDom;

[Serializable]
public class CodeConstructor : CodeMemberMethod
{
	public CodeExpressionCollection BaseConstructorArgs { get; } = new CodeExpressionCollection();


	public CodeExpressionCollection ChainedConstructorArgs { get; } = new CodeExpressionCollection();


	public CodeConstructor()
	{
		base.Name = ".ctor";
	}
}
