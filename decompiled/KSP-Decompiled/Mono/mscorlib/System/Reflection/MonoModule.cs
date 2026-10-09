using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Runtime.Serialization;
using System.Security;
using System.Security.Cryptography.X509Certificates;
using System.Security.Permissions;

namespace System.Reflection;

[Serializable]
[ComDefaultInterface(typeof(_Module))]
[ClassInterface(ClassInterfaceType.None)]
[ComVisible(true)]
internal class MonoModule : RuntimeModule
{
	public override Assembly Assembly => assembly;

	public override string Name => name;

	public override string ScopeName => scopename;

	public override int MDStreamVersion
	{
		get
		{
			if (_impl == IntPtr.Zero)
			{
				throw new NotSupportedException();
			}
			return Module.GetMDStreamVersion(_impl);
		}
	}

	public override Guid ModuleVersionId => GetModuleVersionId();

	public override string FullyQualifiedName
	{
		get
		{
			if (SecurityManager.SecurityEnabled)
			{
				new FileIOPermission(FileIOPermissionAccess.PathDiscovery, fqname).Demand();
			}
			return fqname;
		}
	}

	public override int MetadataToken => Module.get_MetadataToken(this);

	public override bool IsResource()
	{
		return is_resource;
	}

	public override Type[] FindTypes(TypeFilter filter, object filterCriteria)
	{
		List<Type> list = new List<Type>();
		Type[] types = GetTypes();
		foreach (Type type in types)
		{
			if (filter(type, filterCriteria))
			{
				list.Add(type);
			}
		}
		return list.ToArray();
	}

	public override object[] GetCustomAttributes(bool inherit)
	{
		return MonoCustomAttrs.GetCustomAttributes(this, inherit);
	}

	public override object[] GetCustomAttributes(Type attributeType, bool inherit)
	{
		return MonoCustomAttrs.GetCustomAttributes(this, attributeType, inherit);
	}

	public override FieldInfo GetField(string name, BindingFlags bindingAttr)
	{
		if (name == null)
		{
			throw new ArgumentNullException("name");
		}
		if (IsResource())
		{
			return null;
		}
		Type globalType = GetGlobalType();
		if (!(globalType != null))
		{
			return null;
		}
		return globalType.GetField(name, bindingAttr);
	}

	public override FieldInfo[] GetFields(BindingFlags bindingFlags)
	{
		if (IsResource())
		{
			return new FieldInfo[0];
		}
		Type globalType = GetGlobalType();
		if (!(globalType != null))
		{
			return new FieldInfo[0];
		}
		return globalType.GetFields(bindingFlags);
	}

	protected override MethodInfo GetMethodImpl(string name, BindingFlags bindingAttr, Binder binder, CallingConventions callConvention, Type[] types, ParameterModifier[] modifiers)
	{
		if (IsResource())
		{
			return null;
		}
		Type globalType = GetGlobalType();
		if (globalType == null)
		{
			return null;
		}
		if (types == null)
		{
			return globalType.GetMethod(name);
		}
		return globalType.GetMethod(name, bindingAttr, binder, callConvention, types, modifiers);
	}

	public override MethodInfo[] GetMethods(BindingFlags bindingFlags)
	{
		if (IsResource())
		{
			return new MethodInfo[0];
		}
		Type globalType = GetGlobalType();
		if (!(globalType != null))
		{
			return new MethodInfo[0];
		}
		return globalType.GetMethods(bindingFlags);
	}

	public override void GetPEKind(out PortableExecutableKinds peKind, out ImageFileMachine machine)
	{
		base.ModuleHandle.GetPEKind(out peKind, out machine);
	}

	public override Type GetType(string className, bool throwOnError, bool ignoreCase)
	{
		if (className == null)
		{
			throw new ArgumentNullException("className");
		}
		if (className == string.Empty)
		{
			throw new ArgumentException("Type name can't be empty");
		}
		return assembly.InternalGetType(this, className, throwOnError, ignoreCase);
	}

	public override bool IsDefined(Type attributeType, bool inherit)
	{
		return MonoCustomAttrs.IsDefined(this, attributeType, inherit);
	}

	public override FieldInfo ResolveField(int metadataToken, Type[] genericTypeArguments, Type[] genericMethodArguments)
	{
		ResolveTokenError error;
		IntPtr intPtr = Module.ResolveFieldToken(_impl, metadataToken, ptrs_from_types(genericTypeArguments), ptrs_from_types(genericMethodArguments), out error);
		if (intPtr == IntPtr.Zero)
		{
			throw resolve_token_exception(metadataToken, error, "Field");
		}
		return FieldInfo.GetFieldFromHandle(new RuntimeFieldHandle(intPtr));
	}

	public override MemberInfo ResolveMember(int metadataToken, Type[] genericTypeArguments, Type[] genericMethodArguments)
	{
		ResolveTokenError error;
		MemberInfo memberInfo = Module.ResolveMemberToken(_impl, metadataToken, ptrs_from_types(genericTypeArguments), ptrs_from_types(genericMethodArguments), out error);
		if (memberInfo == null)
		{
			throw resolve_token_exception(metadataToken, error, "MemberInfo");
		}
		return memberInfo;
	}

	public override MethodBase ResolveMethod(int metadataToken, Type[] genericTypeArguments, Type[] genericMethodArguments)
	{
		ResolveTokenError error;
		IntPtr intPtr = Module.ResolveMethodToken(_impl, metadataToken, ptrs_from_types(genericTypeArguments), ptrs_from_types(genericMethodArguments), out error);
		if (intPtr == IntPtr.Zero)
		{
			throw resolve_token_exception(metadataToken, error, "MethodBase");
		}
		return MethodBase.GetMethodFromHandleNoGenericCheck(new RuntimeMethodHandle(intPtr));
	}

	public override string ResolveString(int metadataToken)
	{
		ResolveTokenError error;
		string text = Module.ResolveStringToken(_impl, metadataToken, out error);
		if (text == null)
		{
			throw resolve_token_exception(metadataToken, error, "string");
		}
		return text;
	}

	public override Type ResolveType(int metadataToken, Type[] genericTypeArguments, Type[] genericMethodArguments)
	{
		ResolveTokenError error;
		IntPtr intPtr = Module.ResolveTypeToken(_impl, metadataToken, ptrs_from_types(genericTypeArguments), ptrs_from_types(genericMethodArguments), out error);
		if (intPtr == IntPtr.Zero)
		{
			throw resolve_token_exception(metadataToken, error, "Type");
		}
		return Type.GetTypeFromHandle(new RuntimeTypeHandle(intPtr));
	}

	public override byte[] ResolveSignature(int metadataToken)
	{
		ResolveTokenError error;
		byte[] array = Module.ResolveSignature(_impl, metadataToken, out error);
		if (array == null)
		{
			throw resolve_token_exception(metadataToken, error, "signature");
		}
		return array;
	}

	public override void GetObjectData(SerializationInfo info, StreamingContext context)
	{
		if (info == null)
		{
			throw new ArgumentNullException("info");
		}
		UnitySerializationHolder.GetUnitySerializationInfo(info, 5, ScopeName, GetRuntimeAssembly());
	}

	public override X509Certificate GetSignerCertificate()
	{
		try
		{
			return X509Certificate.CreateFromSignedFile(assembly.Location);
		}
		catch
		{
			return null;
		}
	}

	public override Type[] GetTypes()
	{
		return InternalGetTypes();
	}

	public override IList<CustomAttributeData> GetCustomAttributesData()
	{
		return CustomAttributeData.GetCustomAttributes(this);
	}

	internal RuntimeAssembly GetRuntimeAssembly()
	{
		return (RuntimeAssembly)assembly;
	}
}
