using System.Globalization;
using System.Runtime.InteropServices;
using System.Security;
using System.Security.Permissions;
using Unity;

namespace System.Reflection.Emit;

public abstract class TypeBuilder : TypeInfo, _TypeBuilder
{
	public const int UnspecifiedTypeSize = 0;

	public PackingSize PackingSize
	{
		get
		{
			throw new PlatformNotSupportedException();
		}
	}

	public int Size
	{
		get
		{
			throw new PlatformNotSupportedException();
		}
	}

	public override Assembly Assembly
	{
		get
		{
			throw new PlatformNotSupportedException();
		}
	}

	public override string AssemblyQualifiedName
	{
		get
		{
			throw new PlatformNotSupportedException();
		}
	}

	public override Type BaseType
	{
		get
		{
			throw new PlatformNotSupportedException();
		}
	}

	public override string FullName
	{
		get
		{
			throw new PlatformNotSupportedException();
		}
	}

	public override Guid GUID
	{
		get
		{
			throw new PlatformNotSupportedException();
		}
	}

	public override Module Module
	{
		get
		{
			throw new PlatformNotSupportedException();
		}
	}

	public override string Name
	{
		get
		{
			throw new PlatformNotSupportedException();
		}
	}

	public override string Namespace
	{
		get
		{
			throw new PlatformNotSupportedException();
		}
	}

	public TypeToken TypeToken
	{
		get
		{
			ThrowStub.ThrowNotSupportedException();
			return default(TypeToken);
		}
	}

	public override Type UnderlyingSystemType
	{
		get
		{
			ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public void AddInterfaceImplementation(Type interfaceType)
	{
		throw new PlatformNotSupportedException();
	}

	public TypeInfo CreateTypeInfo()
	{
		throw new PlatformNotSupportedException();
	}

	public ConstructorBuilder DefineConstructor(MethodAttributes attributes, CallingConventions callingConvention, Type[] parameterTypes)
	{
		throw new PlatformNotSupportedException();
	}

	public ConstructorBuilder DefineConstructor(MethodAttributes attributes, CallingConventions callingConvention, Type[] parameterTypes, Type[][] requiredCustomModifiers, Type[][] optionalCustomModifiers)
	{
		throw new PlatformNotSupportedException();
	}

	public ConstructorBuilder DefineDefaultConstructor(MethodAttributes attributes)
	{
		throw new PlatformNotSupportedException();
	}

	public EventBuilder DefineEvent(string name, EventAttributes attributes, Type eventtype)
	{
		throw new PlatformNotSupportedException();
	}

	public FieldBuilder DefineField(string fieldName, Type type, FieldAttributes attributes)
	{
		throw new PlatformNotSupportedException();
	}

	public FieldBuilder DefineField(string fieldName, Type type, Type[] requiredCustomModifiers, Type[] optionalCustomModifiers, FieldAttributes attributes)
	{
		throw new PlatformNotSupportedException();
	}

	public GenericTypeParameterBuilder[] DefineGenericParameters(string[] names)
	{
		throw new PlatformNotSupportedException();
	}

	public FieldBuilder DefineInitializedData(string name, byte[] data, FieldAttributes attributes)
	{
		throw new PlatformNotSupportedException();
	}

	public MethodBuilder DefineMethod(string name, MethodAttributes attributes)
	{
		throw new PlatformNotSupportedException();
	}

	public MethodBuilder DefineMethod(string name, MethodAttributes attributes, CallingConventions callingConvention)
	{
		throw new PlatformNotSupportedException();
	}

	public MethodBuilder DefineMethod(string name, MethodAttributes attributes, Type returnType, Type[] parameterTypes)
	{
		throw new PlatformNotSupportedException();
	}

	public MethodBuilder DefineMethod(string name, MethodAttributes attributes, CallingConventions callingConvention, Type returnType, Type[] parameterTypes)
	{
		throw new PlatformNotSupportedException();
	}

	public MethodBuilder DefineMethod(string name, MethodAttributes attributes, CallingConventions callingConvention, Type returnType, Type[] returnTypeRequiredCustomModifiers, Type[] returnTypeOptionalCustomModifiers, Type[] parameterTypes, Type[][] parameterTypeRequiredCustomModifiers, Type[][] parameterTypeOptionalCustomModifiers)
	{
		throw new PlatformNotSupportedException();
	}

	public void DefineMethodOverride(MethodInfo methodInfoBody, MethodInfo methodInfoDeclaration)
	{
		throw new PlatformNotSupportedException();
	}

	public TypeBuilder DefineNestedType(string name)
	{
		throw new PlatformNotSupportedException();
	}

	public TypeBuilder DefineNestedType(string name, TypeAttributes attr)
	{
		throw new PlatformNotSupportedException();
	}

	public TypeBuilder DefineNestedType(string name, TypeAttributes attr, Type parent)
	{
		throw new PlatformNotSupportedException();
	}

	public TypeBuilder DefineNestedType(string name, TypeAttributes attr, Type parent, int typeSize)
	{
		throw new PlatformNotSupportedException();
	}

	public TypeBuilder DefineNestedType(string name, TypeAttributes attr, Type parent, PackingSize packSize)
	{
		throw new PlatformNotSupportedException();
	}

	public TypeBuilder DefineNestedType(string name, TypeAttributes attr, Type parent, Type[] interfaces)
	{
		throw new PlatformNotSupportedException();
	}

	public TypeBuilder DefineNestedType(string name, TypeAttributes attr, Type parent, PackingSize packSize, int typeSize)
	{
		throw new PlatformNotSupportedException();
	}

	public PropertyBuilder DefineProperty(string name, PropertyAttributes attributes, Type returnType, Type[] parameterTypes)
	{
		throw new PlatformNotSupportedException();
	}

	public PropertyBuilder DefineProperty(string name, PropertyAttributes attributes, CallingConventions callingConvention, Type returnType, Type[] parameterTypes)
	{
		throw new PlatformNotSupportedException();
	}

	public PropertyBuilder DefineProperty(string name, PropertyAttributes attributes, Type returnType, Type[] returnTypeRequiredCustomModifiers, Type[] returnTypeOptionalCustomModifiers, Type[] parameterTypes, Type[][] parameterTypeRequiredCustomModifiers, Type[][] parameterTypeOptionalCustomModifiers)
	{
		throw new PlatformNotSupportedException();
	}

	public PropertyBuilder DefineProperty(string name, PropertyAttributes attributes, CallingConventions callingConvention, Type returnType, Type[] returnTypeRequiredCustomModifiers, Type[] returnTypeOptionalCustomModifiers, Type[] parameterTypes, Type[][] parameterTypeRequiredCustomModifiers, Type[][] parameterTypeOptionalCustomModifiers)
	{
		throw new PlatformNotSupportedException();
	}

	public ConstructorBuilder DefineTypeInitializer()
	{
		throw new PlatformNotSupportedException();
	}

	public FieldBuilder DefineUninitializedData(string name, int size, FieldAttributes attributes)
	{
		throw new PlatformNotSupportedException();
	}

	public static ConstructorInfo GetConstructor(Type type, ConstructorInfo constructor)
	{
		throw new PlatformNotSupportedException();
	}

	public static FieldInfo GetField(Type type, FieldInfo field)
	{
		throw new PlatformNotSupportedException();
	}

	public static MethodInfo GetMethod(Type type, MethodInfo method)
	{
		throw new PlatformNotSupportedException();
	}

	public bool IsCreated()
	{
		throw new PlatformNotSupportedException();
	}

	public void SetCustomAttribute(CustomAttributeBuilder customBuilder)
	{
		throw new PlatformNotSupportedException();
	}

	public void SetCustomAttribute(ConstructorInfo con, byte[] binaryAttribute)
	{
		throw new PlatformNotSupportedException();
	}

	public void SetParent(Type parent)
	{
		throw new PlatformNotSupportedException();
	}

	public override Type GetElementType()
	{
		throw new PlatformNotSupportedException();
	}

	[SecuritySafeCritical]
	public void AddDeclarativeSecurity(SecurityAction action, PermissionSet pset)
	{
		ThrowStub.ThrowNotSupportedException();
	}

	[SecuritySafeCritical]
	public Type CreateType()
	{
		ThrowStub.ThrowNotSupportedException();
		return null;
	}

	[SecuritySafeCritical]
	public MethodBuilder DefinePInvokeMethod(string name, string dllName, MethodAttributes attributes, CallingConventions callingConvention, Type returnType, Type[] parameterTypes, CallingConvention nativeCallConv, CharSet nativeCharSet)
	{
		ThrowStub.ThrowNotSupportedException();
		return null;
	}

	[SecuritySafeCritical]
	public MethodBuilder DefinePInvokeMethod(string name, string dllName, string entryName, MethodAttributes attributes, CallingConventions callingConvention, Type returnType, Type[] parameterTypes, CallingConvention nativeCallConv, CharSet nativeCharSet)
	{
		ThrowStub.ThrowNotSupportedException();
		return null;
	}

	[SecuritySafeCritical]
	public MethodBuilder DefinePInvokeMethod(string name, string dllName, string entryName, MethodAttributes attributes, CallingConventions callingConvention, Type returnType, Type[] returnTypeRequiredCustomModifiers, Type[] returnTypeOptionalCustomModifiers, Type[] parameterTypes, Type[][] parameterTypeRequiredCustomModifiers, Type[][] parameterTypeOptionalCustomModifiers, CallingConvention nativeCallConv, CharSet nativeCharSet)
	{
		ThrowStub.ThrowNotSupportedException();
		return null;
	}

	protected override TypeAttributes GetAttributeFlagsImpl()
	{
		ThrowStub.ThrowNotSupportedException();
		return default(TypeAttributes);
	}

	protected override ConstructorInfo GetConstructorImpl(BindingFlags bindingAttr, Binder binder, CallingConventions callConvention, Type[] types, ParameterModifier[] modifiers)
	{
		ThrowStub.ThrowNotSupportedException();
		return null;
	}

	[ComVisible(true)]
	public override ConstructorInfo[] GetConstructors(BindingFlags bindingAttr)
	{
		ThrowStub.ThrowNotSupportedException();
		return null;
	}

	[SecuritySafeCritical]
	public override object[] GetCustomAttributes(bool inherit)
	{
		ThrowStub.ThrowNotSupportedException();
		return null;
	}

	[SecuritySafeCritical]
	public override object[] GetCustomAttributes(Type attributeType, bool inherit)
	{
		ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public override EventInfo GetEvent(string name, BindingFlags bindingAttr)
	{
		ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public override EventInfo[] GetEvents(BindingFlags bindingAttr)
	{
		ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public override FieldInfo GetField(string name, BindingFlags bindingAttr)
	{
		ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public override FieldInfo[] GetFields(BindingFlags bindingAttr)
	{
		ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public override Type GetInterface(string name, bool ignoreCase)
	{
		ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public override Type[] GetInterfaces()
	{
		ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public override MemberInfo[] GetMembers(BindingFlags bindingAttr)
	{
		ThrowStub.ThrowNotSupportedException();
		return null;
	}

	protected override MethodInfo GetMethodImpl(string name, BindingFlags bindingAttr, Binder binder, CallingConventions callConvention, Type[] types, ParameterModifier[] modifiers)
	{
		ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public override MethodInfo[] GetMethods(BindingFlags bindingAttr)
	{
		ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public override Type GetNestedType(string name, BindingFlags bindingAttr)
	{
		ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public override Type[] GetNestedTypes(BindingFlags bindingAttr)
	{
		ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public override PropertyInfo[] GetProperties(BindingFlags bindingAttr)
	{
		ThrowStub.ThrowNotSupportedException();
		return null;
	}

	protected override PropertyInfo GetPropertyImpl(string name, BindingFlags bindingAttr, Binder binder, Type returnType, Type[] types, ParameterModifier[] modifiers)
	{
		ThrowStub.ThrowNotSupportedException();
		return null;
	}

	protected override bool HasElementTypeImpl()
	{
		ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public override object InvokeMember(string name, BindingFlags invokeAttr, Binder binder, object target, object[] args, ParameterModifier[] modifiers, CultureInfo culture, string[] namedParameters)
	{
		ThrowStub.ThrowNotSupportedException();
		return null;
	}

	protected override bool IsArrayImpl()
	{
		ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	protected override bool IsByRefImpl()
	{
		ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	protected override bool IsCOMObjectImpl()
	{
		ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	[SecuritySafeCritical]
	public override bool IsDefined(Type attributeType, bool inherit)
	{
		ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	protected override bool IsPointerImpl()
	{
		ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	protected override bool IsPrimitiveImpl()
	{
		ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	void _TypeBuilder.GetIDsOfNames([In] ref Guid riid, IntPtr rgszNames, uint cNames, uint lcid, IntPtr rgDispId)
	{
		ThrowStub.ThrowNotSupportedException();
	}

	void _TypeBuilder.GetTypeInfo(uint iTInfo, uint lcid, IntPtr ppTInfo)
	{
		ThrowStub.ThrowNotSupportedException();
	}

	void _TypeBuilder.GetTypeInfoCount(out uint pcTInfo)
	{
		ThrowStub.ThrowNotSupportedException();
	}

	void _TypeBuilder.Invoke(uint dispIdMember, [In] ref Guid riid, uint lcid, short wFlags, IntPtr pDispParams, IntPtr pVarResult, IntPtr pExcepInfo, IntPtr puArgErr)
	{
		ThrowStub.ThrowNotSupportedException();
	}
}
