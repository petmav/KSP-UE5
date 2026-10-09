using System.Security.Permissions;
using Unity;

namespace System.CodeDom.Compiler;

[PermissionSet(SecurityAction.InheritanceDemand, Name = "FullTrust")]
[PermissionSet(SecurityAction.LinkDemand, Name = "FullTrust")]
public abstract class CodeCompiler : CodeGenerator, ICodeCompiler
{
	protected abstract string CompilerName { get; }

	protected abstract string FileExtension { get; }

	protected CodeCompiler()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected abstract string CmdArgsFromParameters(CompilerParameters options);

	protected virtual CompilerResults FromDom(CompilerParameters options, CodeCompileUnit e)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	protected virtual CompilerResults FromDomBatch(CompilerParameters options, CodeCompileUnit[] ea)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	protected virtual CompilerResults FromFile(CompilerParameters options, string fileName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	protected virtual CompilerResults FromFileBatch(CompilerParameters options, string[] fileNames)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	protected virtual CompilerResults FromSource(CompilerParameters options, string source)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	protected virtual CompilerResults FromSourceBatch(CompilerParameters options, string[] sources)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	protected virtual string GetResponseFileCmdArgs(CompilerParameters options, string cmdArgs)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	protected static string JoinStringArray(string[] sa, string separator)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	protected abstract void ProcessCompilerOutputLine(CompilerResults results, string line);

	CompilerResults ICodeCompiler.CompileAssemblyFromDom(CompilerParameters options, CodeCompileUnit e)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	CompilerResults ICodeCompiler.CompileAssemblyFromDomBatch(CompilerParameters options, CodeCompileUnit[] ea)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	CompilerResults ICodeCompiler.CompileAssemblyFromFile(CompilerParameters options, string fileName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	CompilerResults ICodeCompiler.CompileAssemblyFromFileBatch(CompilerParameters options, string[] fileNames)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	CompilerResults ICodeCompiler.CompileAssemblyFromSource(CompilerParameters options, string source)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	CompilerResults ICodeCompiler.CompileAssemblyFromSourceBatch(CompilerParameters options, string[] sources)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}
}
