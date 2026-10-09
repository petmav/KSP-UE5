using System.Collections.Generic;
using System.Configuration.Assemblies;
using System.Globalization;
using System.IO;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Runtime.Serialization;
using System.Security;
using System.Security.Permissions;
using System.Security.Policy;
using System.Text;
using System.Threading;
using Mono;

namespace System.Reflection;

[Serializable]
[StructLayout(LayoutKind.Sequential)]
[ComDefaultInterface(typeof(_Assembly))]
[ClassInterface(ClassInterfaceType.None)]
[ComVisible(true)]
public abstract class Assembly : ICustomAttributeProvider, _Assembly, IEvidenceFactory, ISerializable
{
	internal class ResolveEventHolder
	{
		public event ModuleResolveEventHandler ModuleResolve;
	}

	internal class UnmanagedMemoryStreamForModule : UnmanagedMemoryStream
	{
		private Module module;

		public unsafe UnmanagedMemoryStreamForModule(byte* pointer, long length, Module module)
			: base(pointer, length)
		{
			this.module = module;
		}

		protected override void Dispose(bool disposing)
		{
			if (_isOpen)
			{
				module = null;
			}
			base.Dispose(disposing);
		}
	}

	internal IntPtr _mono_assembly;

	private ResolveEventHolder resolve_event_holder;

	private Evidence _evidence;

	internal PermissionSet _minimum;

	internal PermissionSet _optional;

	internal PermissionSet _refuse;

	private PermissionSet _granted;

	private PermissionSet _denied;

	private bool fromByteArray;

	private string assemblyName;

	public virtual string CodeBase => GetCodeBase(escaped: false);

	public virtual string EscapedCodeBase
	{
		[SecuritySafeCritical]
		get
		{
			return GetCodeBase(escaped: true);
		}
	}

	public virtual string FullName => ToString();

	public virtual extern MethodInfo EntryPoint
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		get;
	}

	public virtual Evidence Evidence
	{
		[SecurityPermission(SecurityAction.Demand, ControlEvidence = true)]
		get
		{
			return UnprotectedGetEvidence();
		}
	}

	internal bool FromByteArray
	{
		set
		{
			fromByteArray = value;
		}
	}

	public virtual string Location
	{
		get
		{
			if (fromByteArray)
			{
				return string.Empty;
			}
			string location = get_location();
			if (location != string.Empty && SecurityManager.SecurityEnabled)
			{
				new FileIOPermission(FileIOPermissionAccess.PathDiscovery, location).Demand();
			}
			return location;
		}
	}

	[ComVisible(false)]
	public virtual string ImageRuntimeVersion => InternalImageRuntimeVersion();

	[ComVisible(false)]
	[MonoTODO("Currently it always returns zero")]
	public virtual long HostContext => 0L;

	[ComVisible(false)]
	public virtual extern bool ReflectionOnly
	{
		[MethodImpl(MethodImplOptions.InternalCall)]
		get;
	}

	internal PermissionSet GrantedPermissionSet
	{
		get
		{
			if (_granted == null)
			{
				if (SecurityManager.ResolvingPolicyLevel != null)
				{
					if (SecurityManager.ResolvingPolicyLevel.IsFullTrustAssembly(this))
					{
						return DefaultPolicies.FullTrust;
					}
					return null;
				}
				Resolve();
			}
			return _granted;
		}
	}

	internal PermissionSet DeniedPermissionSet
	{
		get
		{
			if (_granted == null)
			{
				if (SecurityManager.ResolvingPolicyLevel != null)
				{
					if (SecurityManager.ResolvingPolicyLevel.IsFullTrustAssembly(this))
					{
						return null;
					}
					return DefaultPolicies.FullTrust;
				}
				Resolve();
			}
			return _denied;
		}
	}

	public virtual PermissionSet PermissionSet => GrantedPermissionSet;

	public virtual SecurityRuleSet SecurityRuleSet
	{
		get
		{
			throw CreateNIE();
		}
	}

	[MonoTODO]
	public bool IsFullyTrusted => true;

	public virtual Module ManifestModule
	{
		get
		{
			throw CreateNIE();
		}
	}

	public virtual bool GlobalAssemblyCache
	{
		get
		{
			throw CreateNIE();
		}
	}

	public virtual bool IsDynamic => false;

	public virtual IEnumerable<TypeInfo> DefinedTypes
	{
		get
		{
			Type[] types = GetTypes();
			foreach (Type type in types)
			{
				yield return type.GetTypeInfo();
			}
		}
	}

	public virtual IEnumerable<Type> ExportedTypes => GetExportedTypes();

	public virtual IEnumerable<Module> Modules => GetModules();

	public virtual IEnumerable<CustomAttributeData> CustomAttributes => GetCustomAttributesData();

	public virtual event ModuleResolveEventHandler ModuleResolve
	{
		[SecurityPermission(SecurityAction.LinkDemand, ControlAppDomain = true)]
		add
		{
			resolve_event_holder.ModuleResolve += value;
		}
		[SecurityPermission(SecurityAction.LinkDemand, ControlAppDomain = true)]
		remove
		{
			resolve_event_holder.ModuleResolve -= value;
		}
	}

	protected Assembly()
	{
		resolve_event_holder = new ResolveEventHolder();
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	private extern string get_code_base(bool escaped);

	[MethodImpl(MethodImplOptions.InternalCall)]
	private extern string get_fullname();

	[MethodImpl(MethodImplOptions.InternalCall)]
	private extern string get_location();

	[MethodImpl(MethodImplOptions.InternalCall)]
	private extern string InternalImageRuntimeVersion();

	[MethodImpl(MethodImplOptions.InternalCall)]
	internal static extern string GetAotId();

	private string GetCodeBase(bool escaped)
	{
		string text = get_code_base(escaped);
		if (SecurityManager.SecurityEnabled && string.Compare("FILE://", 0, text, 0, 7, ignoreCase: true, CultureInfo.InvariantCulture) == 0)
		{
			string path = text.Substring(7);
			new FileIOPermission(FileIOPermissionAccess.PathDiscovery, path).Demand();
		}
		return text;
	}

	internal Evidence UnprotectedGetEvidence()
	{
		if (_evidence == null)
		{
			lock (this)
			{
				_evidence = Evidence.GetDefaultHostEvidence(this);
			}
		}
		return _evidence;
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	internal extern bool get_global_assembly_cache();

	[SecurityCritical]
	public virtual void GetObjectData(SerializationInfo info, StreamingContext context)
	{
		throw new NotImplementedException();
	}

	public virtual bool IsDefined(Type attributeType, bool inherit)
	{
		return MonoCustomAttrs.IsDefined(this, attributeType, inherit);
	}

	public virtual object[] GetCustomAttributes(bool inherit)
	{
		return MonoCustomAttrs.GetCustomAttributes(this, inherit);
	}

	public virtual object[] GetCustomAttributes(Type attributeType, bool inherit)
	{
		return MonoCustomAttrs.GetCustomAttributes(this, attributeType, inherit);
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	private extern object GetFilesInternal(string name, bool getResourceModules);

	public virtual FileStream[] GetFiles()
	{
		return GetFiles(getResourceModules: false);
	}

	public virtual FileStream[] GetFiles(bool getResourceModules)
	{
		string[] array = (string[])GetFilesInternal(null, getResourceModules);
		if (array == null)
		{
			return EmptyArray<FileStream>.Value;
		}
		string location = Location;
		FileStream[] array2;
		if (location != string.Empty)
		{
			array2 = new FileStream[array.Length + 1];
			array2[0] = new FileStream(location, FileMode.Open, FileAccess.Read);
			for (int i = 0; i < array.Length; i++)
			{
				array2[i + 1] = new FileStream(array[i], FileMode.Open, FileAccess.Read);
			}
		}
		else
		{
			array2 = new FileStream[array.Length];
			for (int j = 0; j < array.Length; j++)
			{
				array2[j] = new FileStream(array[j], FileMode.Open, FileAccess.Read);
			}
		}
		return array2;
	}

	public virtual FileStream GetFile(string name)
	{
		if (name == null)
		{
			throw new ArgumentNullException(null, "Name cannot be null.");
		}
		if (name.Length == 0)
		{
			throw new ArgumentException("Empty name is not valid");
		}
		string text = (string)GetFilesInternal(name, getResourceModules: true);
		if (text != null)
		{
			return new FileStream(text, FileMode.Open, FileAccess.Read);
		}
		return null;
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	internal extern IntPtr GetManifestResourceInternal(string name, out int size, out Module module);

	public unsafe virtual Stream GetManifestResourceStream(string name)
	{
		if (name == null)
		{
			throw new ArgumentNullException("name");
		}
		if (name.Length == 0)
		{
			throw new ArgumentException("String cannot have zero length.", "name");
		}
		ManifestResourceInfo manifestResourceInfo = GetManifestResourceInfo(name);
		if (manifestResourceInfo == null)
		{
			Assembly assembly = AppDomain.CurrentDomain.DoResourceResolve(name, this);
			if (assembly != null && assembly != this)
			{
				return assembly.GetManifestResourceStream(name);
			}
			return null;
		}
		if (manifestResourceInfo.ReferencedAssembly != null)
		{
			return manifestResourceInfo.ReferencedAssembly.GetManifestResourceStream(name);
		}
		if (manifestResourceInfo.FileName != null && manifestResourceInfo.ResourceLocation == (ResourceLocation)0)
		{
			if (fromByteArray)
			{
				throw new FileNotFoundException(manifestResourceInfo.FileName);
			}
			return new FileStream(Path.Combine(Path.GetDirectoryName(Location), manifestResourceInfo.FileName), FileMode.Open, FileAccess.Read);
		}
		int size;
		Module module;
		IntPtr manifestResourceInternal = GetManifestResourceInternal(name, out size, out module);
		if (manifestResourceInternal == (IntPtr)0)
		{
			return null;
		}
		return new UnmanagedMemoryStreamForModule((byte*)(void*)manifestResourceInternal, size, module);
	}

	public virtual Stream GetManifestResourceStream(Type type, string name)
	{
		StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
		return GetManifestResourceStream(type, name, skipSecurityCheck: false, ref stackMark);
	}

	internal Stream GetManifestResourceStream(Type type, string name, bool skipSecurityCheck, ref StackCrawlMark stackMark)
	{
		StringBuilder stringBuilder = new StringBuilder();
		if (type == null)
		{
			if (name == null)
			{
				throw new ArgumentNullException("type");
			}
		}
		else
		{
			string @namespace = type.Namespace;
			if (@namespace != null)
			{
				stringBuilder.Append(@namespace);
				if (name != null)
				{
					stringBuilder.Append(Type.Delimiter);
				}
			}
		}
		if (name != null)
		{
			stringBuilder.Append(name);
		}
		return GetManifestResourceStream(stringBuilder.ToString());
	}

	internal Stream GetManifestResourceStream(string name, ref StackCrawlMark stackMark, bool skipSecurityCheck)
	{
		return GetManifestResourceStream(null, name, skipSecurityCheck, ref stackMark);
	}

	internal string GetSimpleName()
	{
		return GetName(copiedName: true).Name;
	}

	internal byte[] GetPublicKey()
	{
		return GetName(copiedName: true).GetPublicKey();
	}

	internal Version GetVersion()
	{
		return GetName(copiedName: true).Version;
	}

	private AssemblyNameFlags GetFlags()
	{
		return GetName(copiedName: true).Flags;
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	internal virtual extern Type[] GetTypes(bool exportedOnly);

	public virtual Type[] GetTypes()
	{
		return GetTypes(exportedOnly: false);
	}

	public virtual Type[] GetExportedTypes()
	{
		return GetTypes(exportedOnly: true);
	}

	public virtual Type GetType(string name, bool throwOnError)
	{
		return GetType(name, throwOnError, ignoreCase: false);
	}

	public virtual Type GetType(string name)
	{
		return GetType(name, throwOnError: false, ignoreCase: false);
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	internal extern Type InternalGetType(Module module, string name, bool throwOnError, bool ignoreCase);

	[MethodImpl(MethodImplOptions.InternalCall)]
	internal static extern void InternalGetAssemblyName(string assemblyFile, out MonoAssemblyName aname, out string codebase);

	public virtual AssemblyName GetName(bool copiedName)
	{
		throw new NotImplementedException();
	}

	public virtual AssemblyName GetName()
	{
		return GetName(copiedName: false);
	}

	public override string ToString()
	{
		if (assemblyName != null)
		{
			return assemblyName;
		}
		assemblyName = get_fullname();
		return assemblyName;
	}

	public static string CreateQualifiedName(string assemblyName, string typeName)
	{
		return typeName + ", " + assemblyName;
	}

	public static Assembly GetAssembly(Type type)
	{
		if (type != null)
		{
			return type.Assembly;
		}
		throw new ArgumentNullException("type");
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	public static extern Assembly GetEntryAssembly();

	internal Assembly GetSatelliteAssembly(CultureInfo culture, Version version, bool throwOnError)
	{
		if (culture == null)
		{
			throw new ArgumentNullException("culture");
		}
		StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
		string name = GetSimpleName() + ".resources";
		return InternalGetSatelliteAssembly(name, culture, version, throwOnFileNotFound: true, ref stackMark);
	}

	internal RuntimeAssembly InternalGetSatelliteAssembly(string name, CultureInfo culture, Version version, bool throwOnFileNotFound, ref StackCrawlMark stackMark)
	{
		AssemblyName assemblyName = new AssemblyName();
		assemblyName.SetPublicKey(GetPublicKey());
		assemblyName.Flags = GetFlags() | AssemblyNameFlags.PublicKey;
		if (version == null)
		{
			assemblyName.Version = GetVersion();
		}
		else
		{
			assemblyName.Version = version;
		}
		assemblyName.CultureInfo = culture;
		assemblyName.Name = name;
		try
		{
			Assembly assembly = AppDomain.CurrentDomain.LoadSatellite(assemblyName, throwOnError: false);
			if (assembly != null)
			{
				return (RuntimeAssembly)assembly;
			}
		}
		catch (FileNotFoundException)
		{
			Assembly assembly = null;
		}
		if (string.IsNullOrEmpty(Location))
		{
			return null;
		}
		string text = Path.Combine(Path.GetDirectoryName(Location), Path.Combine(culture.Name, assemblyName.Name + ".dll"));
		try
		{
			return (RuntimeAssembly)LoadFrom(text);
		}
		catch
		{
			if (!throwOnFileNotFound && !File.Exists(text))
			{
				return null;
			}
			throw;
		}
	}

	Type _Assembly.GetType()
	{
		return GetType();
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern Assembly LoadFrom(string assemblyFile, bool refonly);

	public static Assembly LoadFrom(string assemblyFile)
	{
		return LoadFrom(assemblyFile, refonly: false);
	}

	[Obsolete]
	public static Assembly LoadFrom(string assemblyFile, Evidence securityEvidence)
	{
		Assembly assembly = LoadFrom(assemblyFile, refonly: false);
		if (assembly != null && securityEvidence != null)
		{
			assembly.Evidence.Merge(securityEvidence);
		}
		return assembly;
	}

	[MonoTODO("This overload is not currently implemented")]
	[Obsolete]
	public static Assembly LoadFrom(string assemblyFile, Evidence securityEvidence, byte[] hashValue, AssemblyHashAlgorithm hashAlgorithm)
	{
		throw new NotImplementedException();
	}

	[MonoTODO]
	public static Assembly LoadFrom(string assemblyFile, byte[] hashValue, AssemblyHashAlgorithm hashAlgorithm)
	{
		throw new NotImplementedException();
	}

	public static Assembly UnsafeLoadFrom(string assemblyFile)
	{
		return LoadFrom(assemblyFile);
	}

	[Obsolete]
	public static Assembly LoadFile(string path, Evidence securityEvidence)
	{
		if (path == null)
		{
			throw new ArgumentNullException("path");
		}
		if (path == string.Empty)
		{
			throw new ArgumentException("Path can't be empty", "path");
		}
		return LoadFrom(path, securityEvidence);
	}

	public static Assembly LoadFile(string path)
	{
		return LoadFile(path, null);
	}

	public static Assembly Load(string assemblyString)
	{
		return AppDomain.CurrentDomain.Load(assemblyString);
	}

	[Obsolete]
	public static Assembly Load(string assemblyString, Evidence assemblySecurity)
	{
		return AppDomain.CurrentDomain.Load(assemblyString, assemblySecurity);
	}

	public static Assembly Load(AssemblyName assemblyRef)
	{
		return AppDomain.CurrentDomain.Load(assemblyRef);
	}

	[Obsolete]
	public static Assembly Load(AssemblyName assemblyRef, Evidence assemblySecurity)
	{
		return AppDomain.CurrentDomain.Load(assemblyRef, assemblySecurity);
	}

	public static Assembly Load(byte[] rawAssembly)
	{
		return AppDomain.CurrentDomain.Load(rawAssembly);
	}

	public static Assembly Load(byte[] rawAssembly, byte[] rawSymbolStore)
	{
		return AppDomain.CurrentDomain.Load(rawAssembly, rawSymbolStore);
	}

	[Obsolete]
	public static Assembly Load(byte[] rawAssembly, byte[] rawSymbolStore, Evidence securityEvidence)
	{
		return AppDomain.CurrentDomain.Load(rawAssembly, rawSymbolStore, securityEvidence);
	}

	[MonoLimitation("Argument securityContextSource is ignored")]
	public static Assembly Load(byte[] rawAssembly, byte[] rawSymbolStore, SecurityContextSource securityContextSource)
	{
		return AppDomain.CurrentDomain.Load(rawAssembly, rawSymbolStore);
	}

	public static Assembly ReflectionOnlyLoad(byte[] rawAssembly)
	{
		return AppDomain.CurrentDomain.Load(rawAssembly, null, null, refonly: true);
	}

	public static Assembly ReflectionOnlyLoad(string assemblyString)
	{
		return AppDomain.CurrentDomain.Load(assemblyString, null, refonly: true);
	}

	public static Assembly ReflectionOnlyLoadFrom(string assemblyFile)
	{
		if (assemblyFile == null)
		{
			throw new ArgumentNullException("assemblyFile");
		}
		return LoadFrom(assemblyFile, refonly: true);
	}

	[Obsolete("This method has been deprecated. Please use Assembly.Load() instead. http://go.microsoft.com/fwlink/?linkid=14202")]
	public static Assembly LoadWithPartialName(string partialName)
	{
		return LoadWithPartialName(partialName, null);
	}

	[MonoTODO("Not implemented")]
	public Module LoadModule(string moduleName, byte[] rawModule)
	{
		throw new NotImplementedException();
	}

	[MonoTODO("Not implemented")]
	public virtual Module LoadModule(string moduleName, byte[] rawModule, byte[] rawSymbolStore)
	{
		throw new NotImplementedException();
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern Assembly load_with_partial_name(string name, Evidence e);

	[Obsolete("This method has been deprecated. Please use Assembly.Load() instead. http://go.microsoft.com/fwlink/?linkid=14202")]
	public static Assembly LoadWithPartialName(string partialName, Evidence securityEvidence)
	{
		return LoadWithPartialName(partialName, securityEvidence, oldBehavior: true);
	}

	internal static Assembly LoadWithPartialName(string partialName, Evidence securityEvidence, bool oldBehavior)
	{
		if (!oldBehavior)
		{
			throw new NotImplementedException();
		}
		if (partialName == null)
		{
			throw new NullReferenceException();
		}
		return load_with_partial_name(partialName, securityEvidence);
	}

	public object CreateInstance(string typeName)
	{
		return CreateInstance(typeName, ignoreCase: false);
	}

	public object CreateInstance(string typeName, bool ignoreCase)
	{
		Type type = GetType(typeName, throwOnError: false, ignoreCase);
		if (type == null)
		{
			return null;
		}
		try
		{
			return Activator.CreateInstance(type);
		}
		catch (InvalidOperationException)
		{
			throw new ArgumentException("It is illegal to invoke a method on a Type loaded via ReflectionOnly methods.");
		}
	}

	public virtual object CreateInstance(string typeName, bool ignoreCase, BindingFlags bindingAttr, Binder binder, object[] args, CultureInfo culture, object[] activationAttributes)
	{
		Type type = GetType(typeName, throwOnError: false, ignoreCase);
		if (type == null)
		{
			return null;
		}
		try
		{
			return Activator.CreateInstance(type, bindingAttr, binder, args, culture, activationAttributes);
		}
		catch (InvalidOperationException)
		{
			throw new ArgumentException("It is illegal to invoke a method on a Type loaded via ReflectionOnly methods.");
		}
	}

	public Module[] GetLoadedModules()
	{
		return GetLoadedModules(getResourceModules: false);
	}

	public Module[] GetModules()
	{
		return GetModules(getResourceModules: false);
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	internal virtual extern Module[] GetModulesInternal();

	[MethodImpl(MethodImplOptions.InternalCall)]
	public virtual extern string[] GetManifestResourceNames();

	[MethodImpl(MethodImplOptions.InternalCall)]
	public static extern Assembly GetExecutingAssembly();

	[MethodImpl(MethodImplOptions.InternalCall)]
	public static extern Assembly GetCallingAssembly();

	[MethodImpl(MethodImplOptions.InternalCall)]
	internal static extern IntPtr InternalGetReferencedAssemblies(Assembly module);

	internal unsafe static AssemblyName[] GetReferencedAssemblies(Assembly module)
	{
		using SafeGPtrArrayHandle safeGPtrArrayHandle = new SafeGPtrArrayHandle(InternalGetReferencedAssemblies(module));
		int length = safeGPtrArrayHandle.Length;
		try
		{
			AssemblyName[] array = new AssemblyName[length];
			for (int i = 0; i < length; i++)
			{
				AssemblyName assemblyName = new AssemblyName();
				MonoAssemblyName* native = (MonoAssemblyName*)(void*)safeGPtrArrayHandle[i];
				assemblyName.FillName(native, null, addVersion: true, addPublickey: false, defaultToken: true, assemblyRef: true);
				array[i] = assemblyName;
			}
			return array;
		}
		finally
		{
			for (int j = 0; j < length; j++)
			{
				MonoAssemblyName* ptr = (MonoAssemblyName*)(void*)safeGPtrArrayHandle[j];
				RuntimeMarshal.FreeAssemblyName(ref *ptr, freeStruct: true);
			}
		}
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	private extern bool GetManifestResourceInfoInternal(string name, ManifestResourceInfo info);

	public virtual ManifestResourceInfo GetManifestResourceInfo(string resourceName)
	{
		if (resourceName == null)
		{
			throw new ArgumentNullException("resourceName");
		}
		if (resourceName.Length == 0)
		{
			throw new ArgumentException("String cannot have zero length.");
		}
		ManifestResourceInfo manifestResourceInfo = new ManifestResourceInfo(null, null, (ResourceLocation)0);
		if (GetManifestResourceInfoInternal(resourceName, manifestResourceInfo))
		{
			return manifestResourceInfo;
		}
		return null;
	}

	internal virtual Module GetManifestModule()
	{
		return GetManifestModuleInternal();
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	internal extern Module GetManifestModuleInternal();

	public override int GetHashCode()
	{
		return base.GetHashCode();
	}

	public override bool Equals(object o)
	{
		if (this == o)
		{
			return true;
		}
		if (o == null)
		{
			return false;
		}
		return ((Assembly)o)._mono_assembly == _mono_assembly;
	}

	internal void Resolve()
	{
		lock (this)
		{
			LoadAssemblyPermissions();
			Evidence evidence = new Evidence(UnprotectedGetEvidence());
			evidence.AddHost(new PermissionRequestEvidence(_minimum, _optional, _refuse));
			_granted = SecurityManager.ResolvePolicy(evidence, _minimum, _optional, _refuse, out _denied);
		}
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	internal static extern bool LoadPermissions(Assembly a, ref IntPtr minimum, ref int minLength, ref IntPtr optional, ref int optLength, ref IntPtr refused, ref int refLength);

	private void LoadAssemblyPermissions()
	{
		IntPtr minimum = IntPtr.Zero;
		IntPtr optional = IntPtr.Zero;
		IntPtr refused = IntPtr.Zero;
		int minLength = 0;
		int optLength = 0;
		int refLength = 0;
		if (LoadPermissions(this, ref minimum, ref minLength, ref optional, ref optLength, ref refused, ref refLength))
		{
			if (minLength > 0)
			{
				byte[] array = new byte[minLength];
				Marshal.Copy(minimum, array, 0, minLength);
				_minimum = SecurityManager.Decode(array);
			}
			if (optLength > 0)
			{
				byte[] array2 = new byte[optLength];
				Marshal.Copy(optional, array2, 0, optLength);
				_optional = SecurityManager.Decode(array2);
			}
			if (refLength > 0)
			{
				byte[] array3 = new byte[refLength];
				Marshal.Copy(refused, array3, 0, refLength);
				_refuse = SecurityManager.Decode(array3);
			}
		}
	}

	private static Exception CreateNIE()
	{
		return new NotImplementedException("Derived classes must implement it");
	}

	public virtual IList<CustomAttributeData> GetCustomAttributesData()
	{
		return CustomAttributeData.GetCustomAttributes(this);
	}

	public virtual Type GetType(string name, bool throwOnError, bool ignoreCase)
	{
		throw CreateNIE();
	}

	public virtual Module GetModule(string name)
	{
		throw CreateNIE();
	}

	public virtual AssemblyName[] GetReferencedAssemblies()
	{
		throw CreateNIE();
	}

	public virtual Module[] GetModules(bool getResourceModules)
	{
		throw CreateNIE();
	}

	[MonoTODO("Always returns the same as GetModules")]
	public virtual Module[] GetLoadedModules(bool getResourceModules)
	{
		throw CreateNIE();
	}

	public virtual Assembly GetSatelliteAssembly(CultureInfo culture)
	{
		throw CreateNIE();
	}

	public virtual Assembly GetSatelliteAssembly(CultureInfo culture, Version version)
	{
		throw CreateNIE();
	}

	public static bool operator ==(Assembly left, Assembly right)
	{
		if ((object)left == right)
		{
			return true;
		}
		if (((object)left == null) ^ ((object)right == null))
		{
			return false;
		}
		return left.Equals(right);
	}

	public static bool operator !=(Assembly left, Assembly right)
	{
		if ((object)left == right)
		{
			return false;
		}
		if (((object)left == null) ^ ((object)right == null))
		{
			return true;
		}
		return !left.Equals(right);
	}
}
