using System.Collections.Generic;
using System.Reflection;

namespace System.CodeDom.Compiler;

public sealed class CompilerInfo
{
	internal readonly IDictionary<string, string> _providerOptions = new Dictionary<string, string>();

	internal string _codeDomProviderTypeName;

	internal CompilerParameters _compilerParams;

	internal string[] _compilerLanguages;

	internal string[] _compilerExtensions;

	private Type _type;

	public Type CodeDomProviderType
	{
		get
		{
			if (_type == null)
			{
				lock (this)
				{
					if (_type == null)
					{
						_type = Type.GetType(_codeDomProviderTypeName);
					}
				}
			}
			return _type;
		}
	}

	public bool IsCodeDomProviderTypeValid => Type.GetType(_codeDomProviderTypeName) != null;

	internal CompilerParameters CompilerParams => _compilerParams;

	internal IDictionary<string, string> ProviderOptions => _providerOptions;

	private CompilerInfo()
	{
	}

	public string[] GetLanguages()
	{
		return CloneCompilerLanguages();
	}

	public string[] GetExtensions()
	{
		return CloneCompilerExtensions();
	}

	public CodeDomProvider CreateProvider()
	{
		if (_providerOptions.Count > 0)
		{
			ConstructorInfo constructor = CodeDomProviderType.GetConstructor(new Type[1] { typeof(IDictionary<string, string>) });
			if (constructor != null)
			{
				return (CodeDomProvider)constructor.Invoke(new object[1] { _providerOptions });
			}
		}
		return (CodeDomProvider)Activator.CreateInstance(CodeDomProviderType);
	}

	public CodeDomProvider CreateProvider(IDictionary<string, string> providerOptions)
	{
		if (providerOptions == null)
		{
			throw new ArgumentNullException("providerOptions");
		}
		ConstructorInfo constructor = CodeDomProviderType.GetConstructor(new Type[1] { typeof(IDictionary<string, string>) });
		if (constructor != null)
		{
			return (CodeDomProvider)constructor.Invoke(new object[1] { providerOptions });
		}
		throw new InvalidOperationException(global::SR.Format("This CodeDomProvider type does not have a constructor that takes providerOptions - \"{0}\"", CodeDomProviderType.ToString()));
	}

	public CompilerParameters CreateDefaultCompilerParameters()
	{
		return CloneCompilerParameters();
	}

	internal CompilerInfo(CompilerParameters compilerParams, string codeDomProviderTypeName, string[] compilerLanguages, string[] compilerExtensions)
	{
		_compilerLanguages = compilerLanguages;
		_compilerExtensions = compilerExtensions;
		_codeDomProviderTypeName = codeDomProviderTypeName;
		_compilerParams = compilerParams ?? new CompilerParameters();
	}

	internal CompilerInfo(CompilerParameters compilerParams, string codeDomProviderTypeName)
	{
		_codeDomProviderTypeName = codeDomProviderTypeName;
		_compilerParams = compilerParams ?? new CompilerParameters();
	}

	public override int GetHashCode()
	{
		return _codeDomProviderTypeName.GetHashCode();
	}

	public override bool Equals(object o)
	{
		if (!(o is CompilerInfo compilerInfo))
		{
			return false;
		}
		if (CodeDomProviderType == compilerInfo.CodeDomProviderType && CompilerParams.WarningLevel == compilerInfo.CompilerParams.WarningLevel && CompilerParams.IncludeDebugInformation == compilerInfo.CompilerParams.IncludeDebugInformation)
		{
			return CompilerParams.CompilerOptions == compilerInfo.CompilerParams.CompilerOptions;
		}
		return false;
	}

	private CompilerParameters CloneCompilerParameters()
	{
		return new CompilerParameters
		{
			IncludeDebugInformation = _compilerParams.IncludeDebugInformation,
			TreatWarningsAsErrors = _compilerParams.TreatWarningsAsErrors,
			WarningLevel = _compilerParams.WarningLevel,
			CompilerOptions = _compilerParams.CompilerOptions
		};
	}

	private string[] CloneCompilerLanguages()
	{
		return (string[])_compilerLanguages.Clone();
	}

	private string[] CloneCompilerExtensions()
	{
		return (string[])_compilerExtensions.Clone();
	}
}
