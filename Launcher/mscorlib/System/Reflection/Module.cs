using System.Collections.Generic;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Runtime.Serialization;
using System.Security;
using System.Security.Cryptography.X509Certificates;
using System.Security.Permissions;
using Unity;

namespace System.Reflection;

[Serializable]
[StructLayout(LayoutKind.Sequential)]
[ComVisible(true)]
[ClassInterface(ClassInterfaceType.None)]
[ComDefaultInterface(typeof(_Module))]
public abstract class Module : ISerializable, ICustomAttributeProvider, _Module
{
	public static readonly TypeFilter FilterTypeName = filter_by_type_name;

	public static readonly TypeFilter FilterTypeNameIgnoreCase = filter_by_type_name_ignore_case;

	internal IntPtr _impl;

	internal Assembly assembly;

	internal string fqname;

	internal string name;

	internal string scopename;

	internal bool is_resource;

	internal int token;

	private const BindingFlags defaultBindingFlags = BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public;

	public ModuleHandle ModuleHandle => new ModuleHandle(_impl);

	internal Guid MvId => GetModuleVersionId();

	public virtual Assembly Assembly
	{
		get
		{
			throw CreateNIE();
		}
	}

	public virtual string Name
	{
		get
		{
			throw CreateNIE();
		}
	}

	public virtual string ScopeName
	{
		get
		{
			throw CreateNIE();
		}
	}

	public virtual int MDStreamVersion
	{
		get
		{
			throw CreateNIE();
		}
	}

	public virtual Guid ModuleVersionId
	{
		get
		{
			throw CreateNIE();
		}
	}

	public virtual string FullyQualifiedName
	{
		get
		{
			throw CreateNIE();
		}
	}

	public virtual int MetadataToken
	{
		get
		{
			throw CreateNIE();
		}
	}

	public virtual IEnumerable<CustomAttributeData> CustomAttributes => GetCustomAttributesData();

	[MethodImpl(MethodImplOptions.InternalCall)]
	internal static extern int get_MetadataToken(Module module);

	[MethodImpl(MethodImplOptions.InternalCall)]
	internal static extern int GetMDStreamVersion(IntPtr module_handle);

	public FieldInfo GetField(string name)
	{
		return GetField(name, BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public);
	}

	public FieldInfo[] GetFields()
	{
		return GetFields(BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public);
	}

	public MethodInfo GetMethod(string name)
	{
		return GetMethodImpl(name, BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public, null, CallingConventions.Any, null, null);
	}

	public MethodInfo GetMethod(string name, Type[] types)
	{
		if (types == null)
		{
			throw new ArgumentNullException("types");
		}
		return GetMethodImpl(name, BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public, null, CallingConventions.Any, types, null);
	}

	public MethodInfo GetMethod(string name, BindingFlags bindingAttr, Binder binder, CallingConventions callConvention, Type[] types, ParameterModifier[] modifiers)
	{
		if (types == null)
		{
			throw new ArgumentNullException("types");
		}
		return GetMethodImpl(name, bindingAttr, binder, callConvention, types, modifiers);
	}

	public MethodInfo[] GetMethods()
	{
		return GetMethods(BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public);
	}

	[SecurityCritical]
	[SecurityPermission(SecurityAction.LinkDemand, SerializationFormatter = true)]
	public virtual void GetObjectData(SerializationInfo info, StreamingContext context)
	{
		throw new NotImplementedException();
	}

	[ComVisible(true)]
	public virtual Type GetType(string className)
	{
		return GetType(className, throwOnError: false, ignoreCase: false);
	}

	[ComVisible(true)]
	public virtual Type GetType(string className, bool ignoreCase)
	{
		return GetType(className, throwOnError: false, ignoreCase);
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	internal extern Type[] InternalGetTypes();

	public override string ToString()
	{
		return name;
	}

	internal Exception resolve_token_exception(int metadataToken, ResolveTokenError error, string tokenType)
	{
		if (error == ResolveTokenError.OutOfRange)
		{
			return new ArgumentOutOfRangeException("metadataToken", $"Token 0x{metadataToken:x} is not valid in the scope of module {name}");
		}
		return new ArgumentException($"Token 0x{metadataToken:x} is not a valid {tokenType} token in the scope of module {name}", "metadataToken");
	}

	internal IntPtr[] ptrs_from_types(Type[] types)
	{
		if (types == null)
		{
			return null;
		}
		IntPtr[] array = new IntPtr[types.Length];
		for (int i = 0; i < types.Length; i++)
		{
			if (types[i] == null)
			{
				throw new ArgumentException();
			}
			array[i] = types[i].TypeHandle.Value;
		}
		return array;
	}

	public FieldInfo ResolveField(int metadataToken)
	{
		return ResolveField(metadataToken, null, null);
	}

	public MemberInfo ResolveMember(int metadataToken)
	{
		return ResolveMember(metadataToken, null, null);
	}

	public MethodBase ResolveMethod(int metadataToken)
	{
		return ResolveMethod(metadataToken, null, null);
	}

	public Type ResolveType(int metadataToken)
	{
		return ResolveType(metadataToken, null, null);
	}

	internal static Type MonoDebugger_ResolveType(Module module, int token)
	{
		ResolveTokenError error;
		IntPtr intPtr = ResolveTypeToken(module._impl, token, null, null, out error);
		if (intPtr == IntPtr.Zero)
		{
			return null;
		}
		return Type.GetTypeFromHandle(new RuntimeTypeHandle(intPtr));
	}

	internal static Guid Mono_GetGuid(Module module)
	{
		return module.GetModuleVersionId();
	}

	internal virtual Guid GetModuleVersionId()
	{
		return new Guid(GetGuidInternal());
	}

	private static bool filter_by_type_name(Type m, object filterCriteria)
	{
		string text = (string)filterCriteria;
		if (text.Length > 0 && text[text.Length - 1] == '*')
		{
			return m.Name.StartsWithOrdinalUnchecked(text.Substring(0, text.Length - 1));
		}
		return m.Name == text;
	}

	private static bool filter_by_type_name_ignore_case(Type m, object filterCriteria)
	{
		string text = (string)filterCriteria;
		if (text.Length > 0 && text[text.Length - 1] == '*')
		{
			return m.Name.StartsWith(text.Substring(0, text.Length - 1), StringComparison.OrdinalIgnoreCase);
		}
		return string.Compare(m.Name, text, StringComparison.OrdinalIgnoreCase) == 0;
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	internal extern IntPtr GetHINSTANCE();

	[MethodImpl(MethodImplOptions.InternalCall)]
	private extern string GetGuidInternal();

	[MethodImpl(MethodImplOptions.InternalCall)]
	internal extern Type GetGlobalType();

	[MethodImpl(MethodImplOptions.InternalCall)]
	internal static extern IntPtr ResolveTypeToken(IntPtr module, int token, IntPtr[] type_args, IntPtr[] method_args, out ResolveTokenError error);

	[MethodImpl(MethodImplOptions.InternalCall)]
	internal static extern IntPtr ResolveMethodToken(IntPtr module, int token, IntPtr[] type_args, IntPtr[] method_args, out ResolveTokenError error);

	[MethodImpl(MethodImplOptions.InternalCall)]
	internal static extern IntPtr ResolveFieldToken(IntPtr module, int token, IntPtr[] type_args, IntPtr[] method_args, out ResolveTokenError error);

	[MethodImpl(MethodImplOptions.InternalCall)]
	internal static extern string ResolveStringToken(IntPtr module, int token, out ResolveTokenError error);

	[MethodImpl(MethodImplOptions.InternalCall)]
	internal static extern MemberInfo ResolveMemberToken(IntPtr module, int token, IntPtr[] type_args, IntPtr[] method_args, out ResolveTokenError error);

	[MethodImpl(MethodImplOptions.InternalCall)]
	internal static extern byte[] ResolveSignature(IntPtr module, int metadataToken, out ResolveTokenError error);

	[MethodImpl(MethodImplOptions.InternalCall)]
	internal static extern void GetPEKind(IntPtr module, out PortableExecutableKinds peKind, out ImageFileMachine machine);

	public override bool Equals(object o)
	{
		return o == this;
	}

	public override int GetHashCode()
	{
		return base.GetHashCode();
	}

	public static bool operator ==(Module left, Module right)
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

	public static bool operator !=(Module left, Module right)
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

	private static Exception CreateNIE()
	{
		return new NotImplementedException("Derived classes must implement it");
	}

	public virtual bool IsResource()
	{
		throw CreateNIE();
	}

	public virtual Type[] FindTypes(TypeFilter filter, object filterCriteria)
	{
		throw CreateNIE();
	}

	public virtual object[] GetCustomAttributes(bool inherit)
	{
		throw CreateNIE();
	}

	public virtual object[] GetCustomAttributes(Type attributeType, bool inherit)
	{
		throw CreateNIE();
	}

	public virtual IList<CustomAttributeData> GetCustomAttributesData()
	{
		throw CreateNIE();
	}

	public virtual FieldInfo GetField(string name, BindingFlags bindingAttr)
	{
		throw CreateNIE();
	}

	public virtual FieldInfo[] GetFields(BindingFlags bindingFlags)
	{
		throw CreateNIE();
	}

	protected virtual MethodInfo GetMethodImpl(string name, BindingFlags bindingAttr, Binder binder, CallingConventions callConvention, Type[] types, ParameterModifier[] modifiers)
	{
		throw CreateNIE();
	}

	public virtual MethodInfo[] GetMethods(BindingFlags bindingFlags)
	{
		throw CreateNIE();
	}

	public virtual void GetPEKind(out PortableExecutableKinds peKind, out ImageFileMachine machine)
	{
		throw CreateNIE();
	}

	[ComVisible(true)]
	public virtual Type GetType(string className, bool throwOnError, bool ignoreCase)
	{
		throw CreateNIE();
	}

	public virtual bool IsDefined(Type attributeType, bool inherit)
	{
		throw CreateNIE();
	}

	public virtual FieldInfo ResolveField(int metadataToken, Type[] genericTypeArguments, Type[] genericMethodArguments)
	{
		throw CreateNIE();
	}

	public virtual MemberInfo ResolveMember(int metadataToken, Type[] genericTypeArguments, Type[] genericMethodArguments)
	{
		throw CreateNIE();
	}

	public virtual MethodBase ResolveMethod(int metadataToken, Type[] genericTypeArguments, Type[] genericMethodArguments)
	{
		throw CreateNIE();
	}

	public virtual byte[] ResolveSignature(int metadataToken)
	{
		throw CreateNIE();
	}

	public virtual string ResolveString(int metadataToken)
	{
		throw CreateNIE();
	}

	public virtual Type ResolveType(int metadataToken, Type[] genericTypeArguments, Type[] genericMethodArguments)
	{
		throw CreateNIE();
	}

	public virtual X509Certificate GetSignerCertificate()
	{
		throw CreateNIE();
	}

	public virtual Type[] GetTypes()
	{
		throw CreateNIE();
	}

	void _Module.GetIDsOfNames([In] ref Guid riid, IntPtr rgszNames, uint cNames, uint lcid, IntPtr rgDispId)
	{
		ThrowStub.ThrowNotSupportedException();
	}

	void _Module.GetTypeInfo(uint iTInfo, uint lcid, IntPtr ppTInfo)
	{
		ThrowStub.ThrowNotSupportedException();
	}

	void _Module.GetTypeInfoCount(out uint pcTInfo)
	{
		ThrowStub.ThrowNotSupportedException();
	}

	void _Module.Invoke(uint dispIdMember, [In] ref Guid riid, uint lcid, short wFlags, IntPtr pDispParams, IntPtr pVarResult, IntPtr pExcepInfo, IntPtr puArgErr)
	{
		ThrowStub.ThrowNotSupportedException();
	}
}
