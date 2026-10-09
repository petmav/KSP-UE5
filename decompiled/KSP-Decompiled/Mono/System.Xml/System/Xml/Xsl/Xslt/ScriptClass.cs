using System.CodeDom;
using System.CodeDom.Compiler;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Threading;

namespace System.Xml.Xsl.Xslt;

internal class ScriptClass
{
	public string ns;

	public CompilerInfo compilerInfo;

	public StringCollection refAssemblies;

	public StringCollection nsImports;

	public CodeTypeDeclaration typeDecl;

	public bool refAssembliesByHref;

	public Dictionary<string, string> scriptUris;

	public string endUri;

	public Location endLoc;

	private static long scriptClassCounter;

	public ISourceLineInfo EndLineInfo => new SourceLineInfo(endUri, endLoc, endLoc);

	public ScriptClass(string ns, CompilerInfo compilerInfo)
	{
		this.ns = ns;
		this.compilerInfo = compilerInfo;
		refAssemblies = new StringCollection();
		nsImports = new StringCollection();
		typeDecl = new CodeTypeDeclaration(GenerateUniqueClassName());
		refAssembliesByHref = false;
		scriptUris = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
	}

	private static string GenerateUniqueClassName()
	{
		return "Script" + Interlocked.Increment(ref scriptClassCounter);
	}

	public void AddScriptBlock(string source, string uriString, int lineNumber, Location end)
	{
		CodeSnippetTypeMember codeSnippetTypeMember = new CodeSnippetTypeMember(source);
		string fileName = SourceLineInfo.GetFileName(uriString);
		if (lineNumber > 0)
		{
			codeSnippetTypeMember.LinePragma = new CodeLinePragma(fileName, lineNumber);
			scriptUris[fileName] = uriString;
		}
		typeDecl.Members.Add(codeSnippetTypeMember);
		endUri = uriString;
		endLoc = end;
	}
}
