using System.Collections.Specialized;
using System.Security.Policy;

namespace System.CodeDom.Compiler;

[Serializable]
public class CompilerParameters
{
	private Evidence _evidence;

	private readonly StringCollection _assemblyNames = new StringCollection();

	private readonly StringCollection _embeddedResources = new StringCollection();

	private readonly StringCollection _linkedResources = new StringCollection();

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

	public string CoreAssemblyFileName { get; set; } = string.Empty;


	public bool GenerateExecutable { get; set; }

	public bool GenerateInMemory { get; set; }

	public StringCollection ReferencedAssemblies => _assemblyNames;

	public string MainClass { get; set; }

	public string OutputAssembly { get; set; }

	public TempFileCollection TempFiles
	{
		get
		{
			return _tempFiles ?? (_tempFiles = new TempFileCollection());
		}
		set
		{
			_tempFiles = value;
		}
	}

	public bool IncludeDebugInformation { get; set; }

	public bool TreatWarningsAsErrors { get; set; }

	public int WarningLevel { get; set; } = -1;


	public string CompilerOptions { get; set; }

	public string Win32Resource { get; set; }

	public StringCollection EmbeddedResources => _embeddedResources;

	public StringCollection LinkedResources => _linkedResources;

	public IntPtr UserToken { get; set; }

	public CompilerParameters()
		: this(null, null)
	{
	}

	public CompilerParameters(string[] assemblyNames)
		: this(assemblyNames, null, includeDebugInformation: false)
	{
	}

	public CompilerParameters(string[] assemblyNames, string outputName)
		: this(assemblyNames, outputName, includeDebugInformation: false)
	{
	}

	public CompilerParameters(string[] assemblyNames, string outputName, bool includeDebugInformation)
	{
		if (assemblyNames != null)
		{
			ReferencedAssemblies.AddRange(assemblyNames);
		}
		OutputAssembly = outputName;
		IncludeDebugInformation = includeDebugInformation;
	}
}
