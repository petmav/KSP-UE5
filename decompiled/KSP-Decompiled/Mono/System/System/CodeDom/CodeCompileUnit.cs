using System.Collections.Specialized;

namespace System.CodeDom;

[Serializable]
public class CodeCompileUnit : CodeObject
{
	private StringCollection _assemblies;

	private CodeAttributeDeclarationCollection _attributes;

	private CodeDirectiveCollection _startDirectives;

	private CodeDirectiveCollection _endDirectives;

	public CodeNamespaceCollection Namespaces { get; } = new CodeNamespaceCollection();


	public StringCollection ReferencedAssemblies => _assemblies ?? (_assemblies = new StringCollection());

	public CodeAttributeDeclarationCollection AssemblyCustomAttributes => _attributes ?? (_attributes = new CodeAttributeDeclarationCollection());

	public CodeDirectiveCollection StartDirectives => _startDirectives ?? (_startDirectives = new CodeDirectiveCollection());

	public CodeDirectiveCollection EndDirectives => _endDirectives ?? (_endDirectives = new CodeDirectiveCollection());
}
