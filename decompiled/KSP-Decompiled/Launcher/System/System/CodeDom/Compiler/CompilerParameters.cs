using System.Collections.Specialized;
using System.Security.Permissions;
using System.Security.Policy;
using Unity;

namespace System.CodeDom.Compiler;

[Serializable]
[PermissionSet(SecurityAction.InheritanceDemand, Name = "FullTrust")]
[PermissionSet(SecurityAction.LinkDemand, Name = "FullTrust")]
public class CompilerParameters
{
	public string CompilerOptions
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public string CoreAssemblyFileName
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public StringCollection EmbeddedResources
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public Evidence Evidence
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		[SecurityPermission(SecurityAction.Demand, ControlEvidence = true)]
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public bool GenerateExecutable
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public bool GenerateInMemory
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public bool IncludeDebugInformation
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public StringCollection LinkedResources
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public string MainClass
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public string OutputAssembly
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public StringCollection ReferencedAssemblies
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public TempFileCollection TempFiles
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public bool TreatWarningsAsErrors
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public IntPtr UserToken
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(IntPtr);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public int WarningLevel
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public string Win32Resource
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public CompilerParameters()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CompilerParameters(string[] assemblyNames)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CompilerParameters(string[] assemblyNames, string outputName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CompilerParameters(string[] assemblyNames, string outputName, bool includeDebugInformation)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
