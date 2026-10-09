using System.Collections.Specialized;
using System.Reflection;
using System.Security.Policy;

namespace System.CodeDom.Compiler;

[Serializable]
public class CompilerResults
{
	private Evidence _evidence;

	private readonly CompilerErrorCollection _errors = new CompilerErrorCollection();

	private readonly StringCollection _output = new StringCollection();

	private Assembly _compiledAssembly;

	private TempFileCollection _tempFiles;

	[Obsolete("CAS policy is obsolete and will be removed in a future release of the .NET Framework. Please see http://go2.microsoft.com/fwlink/?LinkId=131738 for more information.")]
	public Evidence Evidence
	{
		get
		{
			return _evidence?.Clone();
		}
		set
		{
			_evidence = value?.Clone();
		}
	}

	public TempFileCollection TempFiles
	{
		get
		{
			return _tempFiles;
		}
		set
		{
			_tempFiles = value;
		}
	}

	public Assembly CompiledAssembly
	{
		get
		{
			if (_compiledAssembly == null && PathToAssembly != null)
			{
				_compiledAssembly = Assembly.Load(new AssemblyName
				{
					CodeBase = PathToAssembly
				});
			}
			return _compiledAssembly;
		}
		set
		{
			_compiledAssembly = value;
		}
	}

	public CompilerErrorCollection Errors => _errors;

	public StringCollection Output => _output;

	public string PathToAssembly { get; set; }

	public int NativeCompilerReturnValue { get; set; }

	public CompilerResults(TempFileCollection tempFiles)
	{
		_tempFiles = tempFiles;
	}
}
