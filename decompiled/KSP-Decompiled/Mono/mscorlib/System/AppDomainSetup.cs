using System.Collections.Generic;
using System.IO;
using System.Runtime.Hosting;
using System.Runtime.InteropServices;
using System.Runtime.Serialization.Formatters.Binary;
using System.Security;
using System.Security.Policy;
using Unity;

namespace System;

[Serializable]
[StructLayout(LayoutKind.Sequential)]
[ClassInterface(ClassInterfaceType.None)]
[ComVisible(true)]
public sealed class AppDomainSetup : IAppDomainSetup
{
	private string application_base;

	private string application_name;

	private string cache_path;

	private string configuration_file;

	private string dynamic_base;

	private string license_file;

	private string private_bin_path;

	private string private_bin_path_probe;

	private string shadow_copy_directories;

	private string shadow_copy_files;

	private bool publisher_policy;

	private bool path_changed;

	private LoaderOptimization loader_optimization;

	private bool disallow_binding_redirects;

	private bool disallow_code_downloads;

	private ActivationArguments _activationArguments;

	private AppDomainInitializer domain_initializer;

	[NonSerialized]
	private ApplicationTrust application_trust;

	private string[] domain_initializer_args;

	private bool disallow_appbase_probe;

	private byte[] configuration_bytes;

	private byte[] serialized_non_primitives;

	public string ApplicationBase
	{
		[SecuritySafeCritical]
		get
		{
			return GetAppBase(application_base);
		}
		set
		{
			application_base = value;
		}
	}

	public string ApplicationName
	{
		get
		{
			return application_name;
		}
		set
		{
			application_name = value;
		}
	}

	public string CachePath
	{
		[SecuritySafeCritical]
		get
		{
			return cache_path;
		}
		set
		{
			cache_path = value;
		}
	}

	public string ConfigurationFile
	{
		[SecuritySafeCritical]
		get
		{
			if (configuration_file == null)
			{
				return null;
			}
			if (Path.IsPathRooted(configuration_file))
			{
				return configuration_file;
			}
			if (ApplicationBase == null)
			{
				throw new MemberAccessException("The ApplicationBase must be set before retrieving this property.");
			}
			return Path.Combine(ApplicationBase, configuration_file);
		}
		set
		{
			configuration_file = value;
		}
	}

	public bool DisallowPublisherPolicy
	{
		get
		{
			return publisher_policy;
		}
		set
		{
			publisher_policy = value;
		}
	}

	public string DynamicBase
	{
		[SecuritySafeCritical]
		get
		{
			if (dynamic_base == null)
			{
				return null;
			}
			if (Path.IsPathRooted(dynamic_base))
			{
				return dynamic_base;
			}
			if (ApplicationBase == null)
			{
				throw new MemberAccessException("The ApplicationBase must be set before retrieving this property.");
			}
			return Path.Combine(ApplicationBase, dynamic_base);
		}
		[SecuritySafeCritical]
		set
		{
			if (application_name == null)
			{
				throw new MemberAccessException("ApplicationName must be set before the DynamicBase can be set.");
			}
			dynamic_base = Path.Combine(value, ((uint)application_name.GetHashCode()).ToString("x"));
		}
	}

	public string LicenseFile
	{
		[SecuritySafeCritical]
		get
		{
			return license_file;
		}
		set
		{
			license_file = value;
		}
	}

	[MonoLimitation("In Mono this is controlled by the --share-code flag")]
	public LoaderOptimization LoaderOptimization
	{
		get
		{
			return loader_optimization;
		}
		set
		{
			loader_optimization = value;
		}
	}

	public string PrivateBinPath
	{
		[SecuritySafeCritical]
		get
		{
			return private_bin_path;
		}
		set
		{
			private_bin_path = value;
			path_changed = true;
		}
	}

	public string PrivateBinPathProbe
	{
		get
		{
			return private_bin_path_probe;
		}
		set
		{
			private_bin_path_probe = value;
			path_changed = true;
		}
	}

	public string ShadowCopyDirectories
	{
		[SecuritySafeCritical]
		get
		{
			return shadow_copy_directories;
		}
		set
		{
			shadow_copy_directories = value;
		}
	}

	public string ShadowCopyFiles
	{
		get
		{
			return shadow_copy_files;
		}
		set
		{
			shadow_copy_files = value;
		}
	}

	public bool DisallowBindingRedirects
	{
		get
		{
			return disallow_binding_redirects;
		}
		set
		{
			disallow_binding_redirects = value;
		}
	}

	public bool DisallowCodeDownload
	{
		get
		{
			return disallow_code_downloads;
		}
		set
		{
			disallow_code_downloads = value;
		}
	}

	public string TargetFrameworkName { get; set; }

	public ActivationArguments ActivationArguments
	{
		get
		{
			if (_activationArguments != null)
			{
				return _activationArguments;
			}
			DeserializeNonPrimitives();
			return _activationArguments;
		}
		set
		{
			_activationArguments = value;
		}
	}

	[MonoLimitation("it needs to be invoked within the created domain")]
	public AppDomainInitializer AppDomainInitializer
	{
		get
		{
			if (domain_initializer != null)
			{
				return domain_initializer;
			}
			DeserializeNonPrimitives();
			return domain_initializer;
		}
		set
		{
			domain_initializer = value;
		}
	}

	[MonoLimitation("it needs to be used to invoke the initializer within the created domain")]
	public string[] AppDomainInitializerArguments
	{
		get
		{
			return domain_initializer_args;
		}
		set
		{
			domain_initializer_args = value;
		}
	}

	[MonoNotSupported("This property exists but not considered.")]
	public ApplicationTrust ApplicationTrust
	{
		get
		{
			if (application_trust != null)
			{
				return application_trust;
			}
			DeserializeNonPrimitives();
			if (application_trust == null)
			{
				application_trust = new ApplicationTrust();
			}
			return application_trust;
		}
		set
		{
			application_trust = value;
		}
	}

	[MonoNotSupported("This property exists but not considered.")]
	public bool DisallowApplicationBaseProbing
	{
		get
		{
			return disallow_appbase_probe;
		}
		set
		{
			disallow_appbase_probe = value;
		}
	}

	public string AppDomainManagerAssembly
	{
		get
		{
			ThrowStub.ThrowNotSupportedException();
			return null;
		}
		set
		{
			ThrowStub.ThrowNotSupportedException();
		}
	}

	public string AppDomainManagerType
	{
		get
		{
			ThrowStub.ThrowNotSupportedException();
			return null;
		}
		set
		{
			ThrowStub.ThrowNotSupportedException();
		}
	}

	public string[] PartialTrustVisibleAssemblies
	{
		get
		{
			ThrowStub.ThrowNotSupportedException();
			return null;
		}
		set
		{
			ThrowStub.ThrowNotSupportedException();
		}
	}

	public bool SandboxInterop
	{
		get
		{
			ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
		set
		{
			ThrowStub.ThrowNotSupportedException();
		}
	}

	public AppDomainSetup()
	{
	}

	internal AppDomainSetup(AppDomainSetup setup)
	{
		application_base = setup.application_base;
		application_name = setup.application_name;
		cache_path = setup.cache_path;
		configuration_file = setup.configuration_file;
		dynamic_base = setup.dynamic_base;
		license_file = setup.license_file;
		private_bin_path = setup.private_bin_path;
		private_bin_path_probe = setup.private_bin_path_probe;
		shadow_copy_directories = setup.shadow_copy_directories;
		shadow_copy_files = setup.shadow_copy_files;
		publisher_policy = setup.publisher_policy;
		path_changed = setup.path_changed;
		loader_optimization = setup.loader_optimization;
		disallow_binding_redirects = setup.disallow_binding_redirects;
		disallow_code_downloads = setup.disallow_code_downloads;
		_activationArguments = setup._activationArguments;
		domain_initializer = setup.domain_initializer;
		application_trust = setup.application_trust;
		domain_initializer_args = setup.domain_initializer_args;
		disallow_appbase_probe = setup.disallow_appbase_probe;
		configuration_bytes = setup.configuration_bytes;
	}

	public AppDomainSetup(ActivationArguments activationArguments)
	{
		_activationArguments = activationArguments;
	}

	public AppDomainSetup(ActivationContext activationContext)
	{
		_activationArguments = new ActivationArguments(activationContext);
	}

	private static string GetAppBase(string appBase)
	{
		if (appBase == null)
		{
			return null;
		}
		if (appBase.Length >= 8 && appBase.ToLower().StartsWith("file://"))
		{
			appBase = appBase.Substring(7);
			if (Path.DirectorySeparatorChar != '/')
			{
				appBase = appBase.Replace('/', Path.DirectorySeparatorChar);
			}
		}
		appBase = Path.GetFullPath(appBase);
		if (Path.DirectorySeparatorChar != '/')
		{
			bool flag = appBase.StartsWith("\\\\?\\", StringComparison.Ordinal);
			if (appBase.IndexOf(':', flag ? 6 : 2) != -1)
			{
				throw new NotSupportedException("The given path's format is not supported.");
			}
		}
		string directoryName = Path.GetDirectoryName(appBase);
		if (directoryName != null && directoryName.LastIndexOfAny(Path.GetInvalidPathChars()) >= 0)
		{
			throw new ArgumentException(string.Format(Locale.GetText("Invalid path characters in path: '{0}'"), appBase), "appBase");
		}
		string fileName = Path.GetFileName(appBase);
		if (fileName != null && fileName.LastIndexOfAny(Path.GetInvalidFileNameChars()) >= 0)
		{
			throw new ArgumentException(string.Format(Locale.GetText("Invalid filename characters in path: '{0}'"), appBase), "appBase");
		}
		return appBase;
	}

	[MonoNotSupported("This method exists but not considered.")]
	public byte[] GetConfigurationBytes()
	{
		if (configuration_bytes == null)
		{
			return null;
		}
		return configuration_bytes.Clone() as byte[];
	}

	[MonoNotSupported("This method exists but not considered.")]
	public void SetConfigurationBytes(byte[] value)
	{
		configuration_bytes = value;
	}

	private void DeserializeNonPrimitives()
	{
		lock (this)
		{
			if (serialized_non_primitives != null)
			{
				BinaryFormatter binaryFormatter = new BinaryFormatter();
				MemoryStream serializationStream = new MemoryStream(serialized_non_primitives);
				object[] array = (object[])binaryFormatter.Deserialize(serializationStream);
				_activationArguments = (ActivationArguments)array[0];
				domain_initializer = (AppDomainInitializer)array[1];
				application_trust = (ApplicationTrust)array[2];
				serialized_non_primitives = null;
			}
		}
	}

	internal void SerializeNonPrimitives()
	{
		object[] graph = new object[3] { _activationArguments, domain_initializer, application_trust };
		BinaryFormatter binaryFormatter = new BinaryFormatter();
		MemoryStream memoryStream = new MemoryStream();
		binaryFormatter.Serialize(memoryStream, graph);
		serialized_non_primitives = memoryStream.ToArray();
	}

	[MonoTODO("not implemented, does not throw because it's used in testing moonlight")]
	public void SetCompatibilitySwitches(IEnumerable<string> switches)
	{
	}

	[SecurityCritical]
	public void SetNativeFunction(string functionName, int functionVersion, IntPtr functionPointer)
	{
		ThrowStub.ThrowNotSupportedException();
	}
}
