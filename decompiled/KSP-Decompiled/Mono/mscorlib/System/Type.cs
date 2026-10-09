using System.Collections;
using System.Diagnostics;
using System.Globalization;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Security;
using System.Threading;

namespace System;

[Serializable]
[ComVisible(true)]
[ClassInterface(ClassInterfaceType.None)]
[ComDefaultInterface(typeof(_Type))]
public abstract class Type : MemberInfo, _Type, IReflect
{
	public static readonly MemberFilter FilterAttribute = __Filters.Instance.FilterAttribute;

	public static readonly MemberFilter FilterName = __Filters.Instance.FilterName;

	public static readonly MemberFilter FilterNameIgnoreCase = __Filters.Instance.FilterIgnoreCase;

	public static readonly object Missing = System.Reflection.Missing.Value;

	public static readonly char Delimiter = '.';

	public static readonly Type[] EmptyTypes = EmptyArray<Type>.Value;

	private static Binder defaultBinder;

	private const BindingFlags DefaultLookup = BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public;

	internal const BindingFlags DeclaredOnlyLookup = BindingFlags.DeclaredOnly | BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public | BindingFlags.NonPublic;

	internal RuntimeTypeHandle _impl;

	public override MemberTypes MemberType => MemberTypes.TypeInfo;

	public override Type DeclaringType => null;

	public virtual MethodBase DeclaringMethod => null;

	public override Type ReflectedType => null;

	public virtual StructLayoutAttribute StructLayoutAttribute
	{
		get
		{
			throw new NotSupportedException();
		}
	}

	public abstract Guid GUID { get; }

	public static Binder DefaultBinder
	{
		get
		{
			if (defaultBinder == null)
			{
				CreateBinder();
			}
			return defaultBinder;
		}
	}

	public abstract override Module Module { get; }

	public abstract Assembly Assembly { get; }

	public virtual RuntimeTypeHandle TypeHandle
	{
		get
		{
			throw new NotSupportedException();
		}
	}

	public abstract string FullName { get; }

	public abstract string Namespace { get; }

	public abstract string AssemblyQualifiedName { get; }

	public abstract Type BaseType { get; }

	[ComVisible(true)]
	public ConstructorInfo TypeInitializer => GetConstructorImpl(BindingFlags.Static | BindingFlags.Public | BindingFlags.NonPublic, null, CallingConventions.Any, EmptyTypes, null);

	public bool IsNested => DeclaringType != null;

	public TypeAttributes Attributes => GetAttributeFlagsImpl();

	public virtual GenericParameterAttributes GenericParameterAttributes
	{
		get
		{
			throw new NotSupportedException();
		}
	}

	public bool IsVisible
	{
		get
		{
			if (IsGenericParameter)
			{
				return true;
			}
			if (HasElementType)
			{
				return GetElementType().IsVisible;
			}
			Type type = this;
			while (type.IsNested)
			{
				if (!type.IsNestedPublic)
				{
					return false;
				}
				type = type.DeclaringType;
			}
			if (!type.IsPublic)
			{
				return false;
			}
			if (IsGenericType && !IsGenericTypeDefinition)
			{
				Type[] genericArguments = GetGenericArguments();
				for (int i = 0; i < genericArguments.Length; i++)
				{
					if (!genericArguments[i].IsVisible)
					{
						return false;
					}
				}
			}
			return true;
		}
	}

	public bool IsNotPublic => (GetAttributeFlagsImpl() & TypeAttributes.VisibilityMask) == 0;

	public bool IsPublic => (GetAttributeFlagsImpl() & TypeAttributes.VisibilityMask) == TypeAttributes.Public;

	public bool IsNestedPublic => (GetAttributeFlagsImpl() & TypeAttributes.VisibilityMask) == TypeAttributes.NestedPublic;

	public bool IsNestedPrivate => (GetAttributeFlagsImpl() & TypeAttributes.VisibilityMask) == TypeAttributes.NestedPrivate;

	public bool IsNestedFamily => (GetAttributeFlagsImpl() & TypeAttributes.VisibilityMask) == TypeAttributes.NestedFamily;

	public bool IsNestedAssembly => (GetAttributeFlagsImpl() & TypeAttributes.VisibilityMask) == TypeAttributes.NestedAssembly;

	public bool IsNestedFamANDAssem => (GetAttributeFlagsImpl() & TypeAttributes.VisibilityMask) == TypeAttributes.NestedFamANDAssem;

	public bool IsNestedFamORAssem => (GetAttributeFlagsImpl() & TypeAttributes.VisibilityMask) == TypeAttributes.VisibilityMask;

	public bool IsAutoLayout => (GetAttributeFlagsImpl() & TypeAttributes.LayoutMask) == 0;

	public bool IsLayoutSequential => (GetAttributeFlagsImpl() & TypeAttributes.LayoutMask) == TypeAttributes.SequentialLayout;

	public bool IsExplicitLayout => (GetAttributeFlagsImpl() & TypeAttributes.LayoutMask) == TypeAttributes.ExplicitLayout;

	public bool IsClass
	{
		get
		{
			if ((GetAttributeFlagsImpl() & TypeAttributes.ClassSemanticsMask) == 0)
			{
				return !IsValueType;
			}
			return false;
		}
	}

	public bool IsInterface
	{
		[SecuritySafeCritical]
		get
		{
			RuntimeType runtimeType = this as RuntimeType;
			if (runtimeType != null)
			{
				return RuntimeTypeHandle.IsInterface(runtimeType);
			}
			return (GetAttributeFlagsImpl() & TypeAttributes.ClassSemanticsMask) == TypeAttributes.ClassSemanticsMask;
		}
	}

	public bool IsValueType => IsValueTypeImpl();

	public bool IsAbstract => (GetAttributeFlagsImpl() & TypeAttributes.Abstract) != 0;

	public bool IsSealed => (GetAttributeFlagsImpl() & TypeAttributes.Sealed) != 0;

	public virtual bool IsEnum => IsSubclassOf(RuntimeType.EnumType);

	public bool IsSpecialName => (GetAttributeFlagsImpl() & TypeAttributes.SpecialName) != 0;

	public bool IsImport => (GetAttributeFlagsImpl() & TypeAttributes.Import) != 0;

	public virtual bool IsSerializable
	{
		get
		{
			if ((GetAttributeFlagsImpl() & TypeAttributes.Serializable) != 0)
			{
				return true;
			}
			RuntimeType runtimeType = UnderlyingSystemType as RuntimeType;
			if (runtimeType != null)
			{
				return runtimeType.IsSpecialSerializableType();
			}
			return false;
		}
	}

	public bool IsAnsiClass => (GetAttributeFlagsImpl() & TypeAttributes.StringFormatMask) == 0;

	public bool IsUnicodeClass => (GetAttributeFlagsImpl() & TypeAttributes.StringFormatMask) == TypeAttributes.UnicodeClass;

	public bool IsAutoClass => (GetAttributeFlagsImpl() & TypeAttributes.StringFormatMask) == TypeAttributes.AutoClass;

	public bool IsArray => IsArrayImpl();

	internal virtual bool IsSzArray => false;

	public virtual bool IsGenericType => false;

	public virtual bool IsGenericTypeDefinition => false;

	public virtual bool IsConstructedGenericType
	{
		get
		{
			throw new NotImplementedException();
		}
	}

	public virtual bool IsGenericParameter => false;

	public virtual int GenericParameterPosition
	{
		get
		{
			throw new InvalidOperationException(Environment.GetResourceString("Method may only be called on a Type for which Type.IsGenericParameter is true."));
		}
	}

	public virtual bool ContainsGenericParameters
	{
		get
		{
			if (HasElementType)
			{
				return GetRootElementType().ContainsGenericParameters;
			}
			if (IsGenericParameter)
			{
				return true;
			}
			if (!IsGenericType)
			{
				return false;
			}
			Type[] genericArguments = GetGenericArguments();
			for (int i = 0; i < genericArguments.Length; i++)
			{
				if (genericArguments[i].ContainsGenericParameters)
				{
					return true;
				}
			}
			return false;
		}
	}

	public bool IsByRef => IsByRefImpl();

	public bool IsPointer => IsPointerImpl();

	public bool IsPrimitive => IsPrimitiveImpl();

	public bool IsCOMObject => IsCOMObjectImpl();

	internal bool IsWindowsRuntimeObject => IsWindowsRuntimeObjectImpl();

	internal bool IsExportedToWindowsRuntime => IsExportedToWindowsRuntimeImpl();

	public bool HasElementType => HasElementTypeImpl();

	public bool IsContextful => IsContextfulImpl();

	public bool IsMarshalByRef => IsMarshalByRefImpl();

	internal bool HasProxyAttribute => HasProxyAttributeImpl();

	public virtual Type[] GenericTypeArguments
	{
		get
		{
			if (IsGenericType && !IsGenericTypeDefinition)
			{
				return GetGenericArguments();
			}
			return EmptyTypes;
		}
	}

	public virtual bool IsSecurityCritical
	{
		get
		{
			throw new NotImplementedException();
		}
	}

	public virtual bool IsSecuritySafeCritical
	{
		get
		{
			throw new NotImplementedException();
		}
	}

	public virtual bool IsSecurityTransparent
	{
		get
		{
			throw new NotImplementedException();
		}
	}

	internal bool NeedsReflectionSecurityCheck
	{
		get
		{
			if (!IsVisible)
			{
				return true;
			}
			if (IsSecurityCritical && !IsSecuritySafeCritical)
			{
				return true;
			}
			if (IsGenericType)
			{
				Type[] genericArguments = GetGenericArguments();
				for (int i = 0; i < genericArguments.Length; i++)
				{
					if (genericArguments[i].NeedsReflectionSecurityCheck)
					{
						return true;
					}
				}
			}
			else if (IsArray || IsPointer)
			{
				return GetElementType().NeedsReflectionSecurityCheck;
			}
			return false;
		}
	}

	public abstract Type UnderlyingSystemType { get; }

	public virtual bool IsSZArray
	{
		get
		{
			throw new NotImplementedException();
		}
	}

	internal virtual bool IsUserType => true;

	[MethodImpl(MethodImplOptions.NoInlining)]
	public static Type GetType(string typeName, Func<AssemblyName, Assembly> assemblyResolver, Func<Assembly, string, bool, Type> typeResolver)
	{
		StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
		return TypeNameParser.GetType(typeName, assemblyResolver, typeResolver, throwOnError: false, ignoreCase: false, ref stackMark);
	}

	[MethodImpl(MethodImplOptions.NoInlining)]
	public static Type GetType(string typeName, Func<AssemblyName, Assembly> assemblyResolver, Func<Assembly, string, bool, Type> typeResolver, bool throwOnError)
	{
		StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
		return TypeNameParser.GetType(typeName, assemblyResolver, typeResolver, throwOnError, ignoreCase: false, ref stackMark);
	}

	[MethodImpl(MethodImplOptions.NoInlining)]
	public static Type GetType(string typeName, Func<AssemblyName, Assembly> assemblyResolver, Func<Assembly, string, bool, Type> typeResolver, bool throwOnError, bool ignoreCase)
	{
		StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
		return TypeNameParser.GetType(typeName, assemblyResolver, typeResolver, throwOnError, ignoreCase, ref stackMark);
	}

	public virtual Type MakePointerType()
	{
		throw new NotSupportedException();
	}

	public virtual Type MakeByRefType()
	{
		throw new NotSupportedException();
	}

	public virtual Type MakeArrayType()
	{
		throw new NotSupportedException();
	}

	public virtual Type MakeArrayType(int rank)
	{
		throw new NotSupportedException();
	}

	[SecurityCritical]
	public static Type GetTypeFromProgID(string progID)
	{
		return RuntimeType.GetTypeFromProgIDImpl(progID, null, throwOnError: false);
	}

	[SecurityCritical]
	public static Type GetTypeFromProgID(string progID, bool throwOnError)
	{
		return RuntimeType.GetTypeFromProgIDImpl(progID, null, throwOnError);
	}

	[SecurityCritical]
	public static Type GetTypeFromProgID(string progID, string server)
	{
		return RuntimeType.GetTypeFromProgIDImpl(progID, server, throwOnError: false);
	}

	[SecurityCritical]
	public static Type GetTypeFromProgID(string progID, string server, bool throwOnError)
	{
		return RuntimeType.GetTypeFromProgIDImpl(progID, server, throwOnError);
	}

	[SecuritySafeCritical]
	public static Type GetTypeFromCLSID(Guid clsid)
	{
		return RuntimeType.GetTypeFromCLSIDImpl(clsid, null, throwOnError: false);
	}

	[SecuritySafeCritical]
	public static Type GetTypeFromCLSID(Guid clsid, bool throwOnError)
	{
		return RuntimeType.GetTypeFromCLSIDImpl(clsid, null, throwOnError);
	}

	[SecuritySafeCritical]
	public static Type GetTypeFromCLSID(Guid clsid, string server)
	{
		return RuntimeType.GetTypeFromCLSIDImpl(clsid, server, throwOnError: false);
	}

	[SecuritySafeCritical]
	public static Type GetTypeFromCLSID(Guid clsid, string server, bool throwOnError)
	{
		return RuntimeType.GetTypeFromCLSIDImpl(clsid, server, throwOnError);
	}

	public static TypeCode GetTypeCode(Type type)
	{
		if (type == null)
		{
			return TypeCode.Empty;
		}
		return type.GetTypeCodeImpl();
	}

	protected virtual TypeCode GetTypeCodeImpl()
	{
		if (this != UnderlyingSystemType && UnderlyingSystemType != null)
		{
			return GetTypeCode(UnderlyingSystemType);
		}
		return TypeCode.Object;
	}

	private static void CreateBinder()
	{
		if (defaultBinder == null)
		{
			DefaultBinder value = new DefaultBinder();
			Interlocked.CompareExchange(ref defaultBinder, value, null);
		}
	}

	public abstract object InvokeMember(string name, BindingFlags invokeAttr, Binder binder, object target, object[] args, ParameterModifier[] modifiers, CultureInfo culture, string[] namedParameters);

	[DebuggerHidden]
	[DebuggerStepThrough]
	public object InvokeMember(string name, BindingFlags invokeAttr, Binder binder, object target, object[] args, CultureInfo culture)
	{
		return InvokeMember(name, invokeAttr, binder, target, args, null, culture, null);
	}

	[DebuggerStepThrough]
	[DebuggerHidden]
	public object InvokeMember(string name, BindingFlags invokeAttr, Binder binder, object target, object[] args)
	{
		return InvokeMember(name, invokeAttr, binder, target, args, null, null, null);
	}

	internal virtual RuntimeTypeHandle GetTypeHandleInternal()
	{
		return TypeHandle;
	}

	public static RuntimeTypeHandle GetTypeHandle(object o)
	{
		if (o == null)
		{
			throw new ArgumentNullException(null, Environment.GetResourceString("Invalid handle."));
		}
		return new RuntimeTypeHandle((RuntimeType)o.GetType());
	}

	public virtual int GetArrayRank()
	{
		throw new NotSupportedException(Environment.GetResourceString("Derived classes must provide an implementation."));
	}

	[ComVisible(true)]
	public ConstructorInfo GetConstructor(BindingFlags bindingAttr, Binder binder, CallingConventions callConvention, Type[] types, ParameterModifier[] modifiers)
	{
		if (types == null)
		{
			throw new ArgumentNullException("types");
		}
		for (int i = 0; i < types.Length; i++)
		{
			if (types[i] == null)
			{
				throw new ArgumentNullException("types");
			}
		}
		return GetConstructorImpl(bindingAttr, binder, callConvention, types, modifiers);
	}

	[ComVisible(true)]
	public ConstructorInfo GetConstructor(BindingFlags bindingAttr, Binder binder, Type[] types, ParameterModifier[] modifiers)
	{
		if (types == null)
		{
			throw new ArgumentNullException("types");
		}
		for (int i = 0; i < types.Length; i++)
		{
			if (types[i] == null)
			{
				throw new ArgumentNullException("types");
			}
		}
		return GetConstructorImpl(bindingAttr, binder, CallingConventions.Any, types, modifiers);
	}

	[ComVisible(true)]
	public ConstructorInfo GetConstructor(Type[] types)
	{
		return GetConstructor(BindingFlags.Instance | BindingFlags.Public, null, types, null);
	}

	protected abstract ConstructorInfo GetConstructorImpl(BindingFlags bindingAttr, Binder binder, CallingConventions callConvention, Type[] types, ParameterModifier[] modifiers);

	[ComVisible(true)]
	public ConstructorInfo[] GetConstructors()
	{
		return GetConstructors(BindingFlags.Instance | BindingFlags.Public);
	}

	[ComVisible(true)]
	public abstract ConstructorInfo[] GetConstructors(BindingFlags bindingAttr);

	public MethodInfo GetMethod(string name, BindingFlags bindingAttr, Binder binder, CallingConventions callConvention, Type[] types, ParameterModifier[] modifiers)
	{
		if (name == null)
		{
			throw new ArgumentNullException("name");
		}
		if (types == null)
		{
			throw new ArgumentNullException("types");
		}
		for (int i = 0; i < types.Length; i++)
		{
			if (types[i] == null)
			{
				throw new ArgumentNullException("types");
			}
		}
		return GetMethodImpl(name, bindingAttr, binder, callConvention, types, modifiers);
	}

	public MethodInfo GetMethod(string name, BindingFlags bindingAttr, Binder binder, Type[] types, ParameterModifier[] modifiers)
	{
		if (name == null)
		{
			throw new ArgumentNullException("name");
		}
		if (types == null)
		{
			throw new ArgumentNullException("types");
		}
		for (int i = 0; i < types.Length; i++)
		{
			if (types[i] == null)
			{
				throw new ArgumentNullException("types");
			}
		}
		return GetMethodImpl(name, bindingAttr, binder, CallingConventions.Any, types, modifiers);
	}

	public MethodInfo GetMethod(string name, Type[] types, ParameterModifier[] modifiers)
	{
		if (name == null)
		{
			throw new ArgumentNullException("name");
		}
		if (types == null)
		{
			throw new ArgumentNullException("types");
		}
		for (int i = 0; i < types.Length; i++)
		{
			if (types[i] == null)
			{
				throw new ArgumentNullException("types");
			}
		}
		return GetMethodImpl(name, BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public, null, CallingConventions.Any, types, modifiers);
	}

	public MethodInfo GetMethod(string name, Type[] types)
	{
		if (name == null)
		{
			throw new ArgumentNullException("name");
		}
		if (types == null)
		{
			throw new ArgumentNullException("types");
		}
		for (int i = 0; i < types.Length; i++)
		{
			if (types[i] == null)
			{
				throw new ArgumentNullException("types");
			}
		}
		return GetMethodImpl(name, BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public, null, CallingConventions.Any, types, null);
	}

	public MethodInfo GetMethod(string name, BindingFlags bindingAttr)
	{
		if (name == null)
		{
			throw new ArgumentNullException("name");
		}
		return GetMethodImpl(name, bindingAttr, null, CallingConventions.Any, null, null);
	}

	public MethodInfo GetMethod(string name)
	{
		if (name == null)
		{
			throw new ArgumentNullException("name");
		}
		return GetMethodImpl(name, BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public, null, CallingConventions.Any, null, null);
	}

	protected abstract MethodInfo GetMethodImpl(string name, BindingFlags bindingAttr, Binder binder, CallingConventions callConvention, Type[] types, ParameterModifier[] modifiers);

	public MethodInfo[] GetMethods()
	{
		return GetMethods(BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public);
	}

	public abstract MethodInfo[] GetMethods(BindingFlags bindingAttr);

	public abstract FieldInfo GetField(string name, BindingFlags bindingAttr);

	public FieldInfo GetField(string name)
	{
		return GetField(name, BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public);
	}

	public FieldInfo[] GetFields()
	{
		return GetFields(BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public);
	}

	public abstract FieldInfo[] GetFields(BindingFlags bindingAttr);

	public Type GetInterface(string name)
	{
		return GetInterface(name, ignoreCase: false);
	}

	public abstract Type GetInterface(string name, bool ignoreCase);

	public abstract Type[] GetInterfaces();

	public virtual Type[] FindInterfaces(TypeFilter filter, object filterCriteria)
	{
		if (filter == null)
		{
			throw new ArgumentNullException("filter");
		}
		Type[] interfaces = GetInterfaces();
		int num = 0;
		for (int i = 0; i < interfaces.Length; i++)
		{
			if (!filter(interfaces[i], filterCriteria))
			{
				interfaces[i] = null;
			}
			else
			{
				num++;
			}
		}
		if (num == interfaces.Length)
		{
			return interfaces;
		}
		Type[] array = new Type[num];
		num = 0;
		for (int j = 0; j < interfaces.Length; j++)
		{
			if (interfaces[j] != null)
			{
				array[num++] = interfaces[j];
			}
		}
		return array;
	}

	public EventInfo GetEvent(string name)
	{
		return GetEvent(name, BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public);
	}

	public abstract EventInfo GetEvent(string name, BindingFlags bindingAttr);

	public virtual EventInfo[] GetEvents()
	{
		return GetEvents(BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public);
	}

	public abstract EventInfo[] GetEvents(BindingFlags bindingAttr);

	public PropertyInfo GetProperty(string name, BindingFlags bindingAttr, Binder binder, Type returnType, Type[] types, ParameterModifier[] modifiers)
	{
		if (name == null)
		{
			throw new ArgumentNullException("name");
		}
		if (types == null)
		{
			throw new ArgumentNullException("types");
		}
		return GetPropertyImpl(name, bindingAttr, binder, returnType, types, modifiers);
	}

	public PropertyInfo GetProperty(string name, Type returnType, Type[] types, ParameterModifier[] modifiers)
	{
		if (name == null)
		{
			throw new ArgumentNullException("name");
		}
		if (types == null)
		{
			throw new ArgumentNullException("types");
		}
		return GetPropertyImpl(name, BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public, null, returnType, types, modifiers);
	}

	public PropertyInfo GetProperty(string name, BindingFlags bindingAttr)
	{
		if (name == null)
		{
			throw new ArgumentNullException("name");
		}
		return GetPropertyImpl(name, bindingAttr, null, null, null, null);
	}

	public PropertyInfo GetProperty(string name, Type returnType, Type[] types)
	{
		if (name == null)
		{
			throw new ArgumentNullException("name");
		}
		if (types == null)
		{
			throw new ArgumentNullException("types");
		}
		return GetPropertyImpl(name, BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public, null, returnType, types, null);
	}

	public PropertyInfo GetProperty(string name, Type[] types)
	{
		if (name == null)
		{
			throw new ArgumentNullException("name");
		}
		if (types == null)
		{
			throw new ArgumentNullException("types");
		}
		return GetPropertyImpl(name, BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public, null, null, types, null);
	}

	public PropertyInfo GetProperty(string name, Type returnType)
	{
		if (name == null)
		{
			throw new ArgumentNullException("name");
		}
		if (returnType == null)
		{
			throw new ArgumentNullException("returnType");
		}
		return GetPropertyImpl(name, BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public, null, returnType, null, null);
	}

	internal PropertyInfo GetProperty(string name, BindingFlags bindingAttr, Type returnType)
	{
		if (name == null)
		{
			throw new ArgumentNullException("name");
		}
		if (returnType == null)
		{
			throw new ArgumentNullException("returnType");
		}
		return GetPropertyImpl(name, bindingAttr, null, returnType, null, null);
	}

	public PropertyInfo GetProperty(string name)
	{
		if (name == null)
		{
			throw new ArgumentNullException("name");
		}
		return GetPropertyImpl(name, BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public, null, null, null, null);
	}

	protected abstract PropertyInfo GetPropertyImpl(string name, BindingFlags bindingAttr, Binder binder, Type returnType, Type[] types, ParameterModifier[] modifiers);

	public abstract PropertyInfo[] GetProperties(BindingFlags bindingAttr);

	public PropertyInfo[] GetProperties()
	{
		return GetProperties(BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public);
	}

	public Type[] GetNestedTypes()
	{
		return GetNestedTypes(BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public);
	}

	public abstract Type[] GetNestedTypes(BindingFlags bindingAttr);

	public Type GetNestedType(string name)
	{
		return GetNestedType(name, BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public);
	}

	public abstract Type GetNestedType(string name, BindingFlags bindingAttr);

	public MemberInfo[] GetMember(string name)
	{
		return GetMember(name, BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public);
	}

	public virtual MemberInfo[] GetMember(string name, BindingFlags bindingAttr)
	{
		return GetMember(name, MemberTypes.All, bindingAttr);
	}

	public virtual MemberInfo[] GetMember(string name, MemberTypes type, BindingFlags bindingAttr)
	{
		throw new NotSupportedException(Environment.GetResourceString("Derived classes must provide an implementation."));
	}

	public MemberInfo[] GetMembers()
	{
		return GetMembers(BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public);
	}

	public abstract MemberInfo[] GetMembers(BindingFlags bindingAttr);

	public virtual MemberInfo[] GetDefaultMembers()
	{
		throw new NotImplementedException();
	}

	public virtual MemberInfo[] FindMembers(MemberTypes memberType, BindingFlags bindingAttr, MemberFilter filter, object filterCriteria)
	{
		MethodInfo[] array = null;
		ConstructorInfo[] array2 = null;
		FieldInfo[] array3 = null;
		PropertyInfo[] array4 = null;
		EventInfo[] array5 = null;
		Type[] array6 = null;
		int num = 0;
		int num2 = 0;
		if ((memberType & MemberTypes.Method) != 0)
		{
			array = GetMethods(bindingAttr);
			if (filter != null)
			{
				for (num = 0; num < array.Length; num++)
				{
					if (!filter(array[num], filterCriteria))
					{
						array[num] = null;
					}
					else
					{
						num2++;
					}
				}
			}
			else
			{
				num2 += array.Length;
			}
		}
		if ((memberType & MemberTypes.Constructor) != 0)
		{
			array2 = GetConstructors(bindingAttr);
			if (filter != null)
			{
				for (num = 0; num < array2.Length; num++)
				{
					if (!filter(array2[num], filterCriteria))
					{
						array2[num] = null;
					}
					else
					{
						num2++;
					}
				}
			}
			else
			{
				num2 += array2.Length;
			}
		}
		if ((memberType & MemberTypes.Field) != 0)
		{
			array3 = GetFields(bindingAttr);
			if (filter != null)
			{
				for (num = 0; num < array3.Length; num++)
				{
					if (!filter(array3[num], filterCriteria))
					{
						array3[num] = null;
					}
					else
					{
						num2++;
					}
				}
			}
			else
			{
				num2 += array3.Length;
			}
		}
		if ((memberType & MemberTypes.Property) != 0)
		{
			array4 = GetProperties(bindingAttr);
			if (filter != null)
			{
				for (num = 0; num < array4.Length; num++)
				{
					if (!filter(array4[num], filterCriteria))
					{
						array4[num] = null;
					}
					else
					{
						num2++;
					}
				}
			}
			else
			{
				num2 += array4.Length;
			}
		}
		if ((memberType & MemberTypes.Event) != 0)
		{
			array5 = GetEvents(bindingAttr);
			if (filter != null)
			{
				for (num = 0; num < array5.Length; num++)
				{
					if (!filter(array5[num], filterCriteria))
					{
						array5[num] = null;
					}
					else
					{
						num2++;
					}
				}
			}
			else
			{
				num2 += array5.Length;
			}
		}
		if ((memberType & MemberTypes.NestedType) != 0)
		{
			array6 = GetNestedTypes(bindingAttr);
			if (filter != null)
			{
				for (num = 0; num < array6.Length; num++)
				{
					if (!filter(array6[num], filterCriteria))
					{
						array6[num] = null;
					}
					else
					{
						num2++;
					}
				}
			}
			else
			{
				num2 += array6.Length;
			}
		}
		MemberInfo[] array7 = new MemberInfo[num2];
		num2 = 0;
		if (array != null)
		{
			for (num = 0; num < array.Length; num++)
			{
				if (array[num] != null)
				{
					array7[num2++] = array[num];
				}
			}
		}
		if (array2 != null)
		{
			for (num = 0; num < array2.Length; num++)
			{
				if (array2[num] != null)
				{
					array7[num2++] = array2[num];
				}
			}
		}
		if (array3 != null)
		{
			for (num = 0; num < array3.Length; num++)
			{
				if (array3[num] != null)
				{
					array7[num2++] = array3[num];
				}
			}
		}
		if (array4 != null)
		{
			for (num = 0; num < array4.Length; num++)
			{
				if (array4[num] != null)
				{
					array7[num2++] = array4[num];
				}
			}
		}
		if (array5 != null)
		{
			for (num = 0; num < array5.Length; num++)
			{
				if (array5[num] != null)
				{
					array7[num2++] = array5[num];
				}
			}
		}
		if (array6 != null)
		{
			for (num = 0; num < array6.Length; num++)
			{
				if (array6[num] != null)
				{
					array7[num2++] = array6[num];
				}
			}
		}
		return array7;
	}

	public virtual Type[] GetGenericParameterConstraints()
	{
		if (!IsGenericParameter)
		{
			throw new InvalidOperationException(Environment.GetResourceString("Method may only be called on a Type for which Type.IsGenericParameter is true."));
		}
		throw new InvalidOperationException();
	}

	protected virtual bool IsValueTypeImpl()
	{
		return IsSubclassOf(RuntimeType.ValueType);
	}

	protected abstract TypeAttributes GetAttributeFlagsImpl();

	protected abstract bool IsArrayImpl();

	protected abstract bool IsByRefImpl();

	protected abstract bool IsPointerImpl();

	protected abstract bool IsPrimitiveImpl();

	protected abstract bool IsCOMObjectImpl();

	internal virtual bool IsWindowsRuntimeObjectImpl()
	{
		throw new NotImplementedException();
	}

	internal virtual bool IsExportedToWindowsRuntimeImpl()
	{
		throw new NotImplementedException();
	}

	public virtual Type MakeGenericType(params Type[] typeArguments)
	{
		throw new NotSupportedException(Environment.GetResourceString("Derived classes must provide an implementation."));
	}

	protected virtual bool IsContextfulImpl()
	{
		return typeof(ContextBoundObject).IsAssignableFrom(this);
	}

	protected virtual bool IsMarshalByRefImpl()
	{
		return typeof(MarshalByRefObject).IsAssignableFrom(this);
	}

	internal virtual bool HasProxyAttributeImpl()
	{
		return false;
	}

	public abstract Type GetElementType();

	public virtual Type[] GetGenericArguments()
	{
		throw new NotSupportedException(Environment.GetResourceString("Derived classes must provide an implementation."));
	}

	public virtual Type GetGenericTypeDefinition()
	{
		throw new NotSupportedException(Environment.GetResourceString("Derived classes must provide an implementation."));
	}

	protected abstract bool HasElementTypeImpl();

	internal Type GetRootElementType()
	{
		Type type = this;
		while (type.HasElementType)
		{
			type = type.GetElementType();
		}
		return type;
	}

	public virtual string[] GetEnumNames()
	{
		if (!IsEnum)
		{
			throw new ArgumentException(Environment.GetResourceString("Type provided must be an Enum."), "enumType");
		}
		GetEnumData(out var enumNames, out var _);
		return enumNames;
	}

	public virtual Array GetEnumValues()
	{
		if (!IsEnum)
		{
			throw new ArgumentException(Environment.GetResourceString("Type provided must be an Enum."), "enumType");
		}
		throw new NotImplementedException();
	}

	private Array GetEnumRawConstantValues()
	{
		GetEnumData(out var _, out var enumValues);
		return enumValues;
	}

	private void GetEnumData(out string[] enumNames, out Array enumValues)
	{
		FieldInfo[] fields = GetFields(BindingFlags.Static | BindingFlags.Public | BindingFlags.NonPublic);
		object[] array = new object[fields.Length];
		string[] array2 = new string[fields.Length];
		for (int i = 0; i < fields.Length; i++)
		{
			array2[i] = fields[i].Name;
			array[i] = fields[i].GetRawConstantValue();
		}
		IComparer @default = Comparer.Default;
		for (int j = 1; j < array.Length; j++)
		{
			int num = j;
			string text = array2[j];
			object obj = array[j];
			bool flag = false;
			while (@default.Compare(array[num - 1], obj) > 0)
			{
				array2[num] = array2[num - 1];
				array[num] = array[num - 1];
				num--;
				flag = true;
				if (num == 0)
				{
					break;
				}
			}
			if (flag)
			{
				array2[num] = text;
				array[num] = obj;
			}
		}
		enumNames = array2;
		enumValues = array;
	}

	public virtual Type GetEnumUnderlyingType()
	{
		if (!IsEnum)
		{
			throw new ArgumentException(Environment.GetResourceString("Type provided must be an Enum."), "enumType");
		}
		FieldInfo[] fields = GetFields(BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic);
		if (fields == null || fields.Length != 1)
		{
			throw new ArgumentException(Environment.GetResourceString("The Enum type should contain one and only one instance field."), "enumType");
		}
		return fields[0].FieldType;
	}

	public virtual bool IsEnumDefined(object value)
	{
		if (value == null)
		{
			throw new ArgumentNullException("value");
		}
		if (!IsEnum)
		{
			throw new ArgumentException(Environment.GetResourceString("Type provided must be an Enum."), "enumType");
		}
		Type type = value.GetType();
		if (type.IsEnum)
		{
			if (!type.IsEquivalentTo(this))
			{
				throw new ArgumentException(Environment.GetResourceString("Object must be the same type as the enum. The type passed in was '{0}'; the enum type was '{1}'.", type.ToString(), ToString()));
			}
			type = type.GetEnumUnderlyingType();
		}
		if (type == typeof(string))
		{
			if (Array.IndexOf(GetEnumNames(), value) >= 0)
			{
				return true;
			}
			return false;
		}
		if (IsIntegerType(type))
		{
			Type enumUnderlyingType = GetEnumUnderlyingType();
			if (enumUnderlyingType.GetTypeCodeImpl() != type.GetTypeCodeImpl())
			{
				throw new ArgumentException(Environment.GetResourceString("Enum underlying type and the object must be same type or object must be a String. Type passed in was '{0}'; the enum underlying type was '{1}'.", type.ToString(), enumUnderlyingType.ToString()));
			}
			return BinarySearch(GetEnumRawConstantValues(), value) >= 0;
		}
		if (CompatibilitySwitches.IsAppEarlierThanWindowsPhone8)
		{
			throw new ArgumentException(Environment.GetResourceString("Enum underlying type and the object must be same type or object must be a String. Type passed in was '{0}'; the enum underlying type was '{1}'.", type.ToString(), GetEnumUnderlyingType()));
		}
		throw new InvalidOperationException(Environment.GetResourceString("Unknown enum type."));
	}

	public virtual string GetEnumName(object value)
	{
		if (value == null)
		{
			throw new ArgumentNullException("value");
		}
		if (!IsEnum)
		{
			throw new ArgumentException(Environment.GetResourceString("Type provided must be an Enum."), "enumType");
		}
		Type type = value.GetType();
		if (!type.IsEnum && !IsIntegerType(type))
		{
			throw new ArgumentException(Environment.GetResourceString("The value passed in must be an enum base or an underlying type for an enum, such as an Int32."), "value");
		}
		int num = BinarySearch(GetEnumRawConstantValues(), value);
		if (num >= 0)
		{
			return GetEnumNames()[num];
		}
		return null;
	}

	private static int BinarySearch(Array array, object value)
	{
		ulong[] array2 = new ulong[array.Length];
		for (int i = 0; i < array.Length; i++)
		{
			array2[i] = Enum.ToUInt64(array.GetValue(i));
		}
		ulong value2 = Enum.ToUInt64(value);
		return Array.BinarySearch(array2, value2);
	}

	internal static bool IsIntegerType(Type t)
	{
		if (!(t == typeof(int)) && !(t == typeof(short)) && !(t == typeof(ushort)) && !(t == typeof(byte)) && !(t == typeof(sbyte)) && !(t == typeof(uint)) && !(t == typeof(long)) && !(t == typeof(ulong)) && !(t == typeof(char)))
		{
			return t == typeof(bool);
		}
		return true;
	}

	[ComVisible(true)]
	public virtual bool IsSubclassOf(Type c)
	{
		Type type = this;
		if (type == c)
		{
			return false;
		}
		while (type != null)
		{
			if (type == c)
			{
				return true;
			}
			type = type.BaseType;
		}
		return false;
	}

	public virtual bool IsInstanceOfType(object o)
	{
		if (o == null)
		{
			return false;
		}
		return IsAssignableFrom(o.GetType());
	}

	public virtual bool IsAssignableFrom(Type c)
	{
		if (c == null)
		{
			return false;
		}
		if (this == c)
		{
			return true;
		}
		RuntimeType runtimeType = UnderlyingSystemType as RuntimeType;
		if (runtimeType != null)
		{
			return runtimeType.IsAssignableFrom(c);
		}
		if (c.IsSubclassOf(this))
		{
			return true;
		}
		if (IsInterface)
		{
			return c.ImplementInterface(this);
		}
		if (IsGenericParameter)
		{
			Type[] genericParameterConstraints = GetGenericParameterConstraints();
			for (int i = 0; i < genericParameterConstraints.Length; i++)
			{
				if (!genericParameterConstraints[i].IsAssignableFrom(c))
				{
					return false;
				}
			}
			return true;
		}
		return false;
	}

	public virtual bool IsEquivalentTo(Type other)
	{
		return this == other;
	}

	internal bool ImplementInterface(Type ifaceType)
	{
		Type type = this;
		while (type != null)
		{
			Type[] interfaces = type.GetInterfaces();
			if (interfaces != null)
			{
				for (int i = 0; i < interfaces.Length; i++)
				{
					if (interfaces[i] == ifaceType || (interfaces[i] != null && interfaces[i].ImplementInterface(ifaceType)))
					{
						return true;
					}
				}
			}
			type = type.BaseType;
		}
		return false;
	}

	internal string FormatTypeName()
	{
		return FormatTypeName(serialization: false);
	}

	internal virtual string FormatTypeName(bool serialization)
	{
		throw new NotImplementedException();
	}

	public override string ToString()
	{
		return "Type: " + Name;
	}

	public static Type[] GetTypeArray(object[] args)
	{
		if (args == null)
		{
			throw new ArgumentNullException("args");
		}
		Type[] array = new Type[args.Length];
		for (int i = 0; i < array.Length; i++)
		{
			if (args[i] == null)
			{
				throw new ArgumentNullException();
			}
			array[i] = args[i].GetType();
		}
		return array;
	}

	public override bool Equals(object o)
	{
		if (o == null)
		{
			return false;
		}
		return Equals(o as Type);
	}

	public virtual bool Equals(Type o)
	{
		if ((object)o == null)
		{
			return false;
		}
		return (object)UnderlyingSystemType == o.UnderlyingSystemType;
	}

	public static bool operator ==(Type left, Type right)
	{
		return (object)left == right;
	}

	public static bool operator !=(Type left, Type right)
	{
		return (object)left != right;
	}

	public override int GetHashCode()
	{
		Type underlyingSystemType = UnderlyingSystemType;
		if ((object)underlyingSystemType != this)
		{
			return underlyingSystemType.GetHashCode();
		}
		return base.GetHashCode();
	}

	[ComVisible(true)]
	public virtual InterfaceMapping GetInterfaceMap(Type interfaceType)
	{
		throw new NotSupportedException(Environment.GetResourceString("Derived classes must provide an implementation."));
	}

	public new Type GetType()
	{
		return base.GetType();
	}

	void _Type.GetTypeInfoCount(out uint pcTInfo)
	{
		throw new NotImplementedException();
	}

	void _Type.GetTypeInfo(uint iTInfo, uint lcid, IntPtr ppTInfo)
	{
		throw new NotImplementedException();
	}

	void _Type.GetIDsOfNames([In] ref Guid riid, IntPtr rgszNames, uint cNames, uint lcid, IntPtr rgDispId)
	{
		throw new NotImplementedException();
	}

	void _Type.Invoke(uint dispIdMember, [In] ref Guid riid, uint lcid, short wFlags, IntPtr pDispParams, IntPtr pVarResult, IntPtr pExcepInfo, IntPtr puArgErr)
	{
		throw new NotImplementedException();
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern Type internal_from_name(string name, bool throwOnError, bool ignoreCase);

	public static Type GetType(string typeName)
	{
		return GetType(typeName, throwOnError: false, ignoreCase: false);
	}

	public static Type GetType(string typeName, bool throwOnError)
	{
		return GetType(typeName, throwOnError, ignoreCase: false);
	}

	public static Type GetType(string typeName, bool throwOnError, bool ignoreCase)
	{
		if (typeName == null)
		{
			throw new ArgumentNullException("TypeName");
		}
		if (typeName == string.Empty)
		{
			if (throwOnError)
			{
				throw new TypeLoadException("A null or zero length string does not represent a valid Type.");
			}
			return null;
		}
		Type type = internal_from_name(typeName, throwOnError, ignoreCase);
		if (throwOnError && type == null)
		{
			throw new TypeLoadException("Error loading '" + typeName + "'");
		}
		return type;
	}

	public static Type ReflectionOnlyGetType(string typeName, bool throwIfNotFound, bool ignoreCase)
	{
		if (typeName == null)
		{
			throw new ArgumentNullException("typeName");
		}
		if (typeName == string.Empty && throwIfNotFound)
		{
			throw new TypeLoadException("A null or zero length string does not represent a valid Type");
		}
		int num = typeName.IndexOf(',');
		if (num < 0 || num == 0 || num == typeName.Length - 1)
		{
			throw new ArgumentException("Assembly qualifed type name is required", "typeName");
		}
		string assemblyString = typeName.Substring(num + 1);
		Assembly assembly;
		try
		{
			assembly = Assembly.ReflectionOnlyLoad(assemblyString);
		}
		catch
		{
			if (throwIfNotFound)
			{
				throw;
			}
			return null;
		}
		return assembly.GetType(typeName.Substring(0, num), throwIfNotFound, ignoreCase);
	}

	internal virtual Type InternalResolve()
	{
		return UnderlyingSystemType;
	}

	internal virtual Type RuntimeResolve()
	{
		throw new NotImplementedException();
	}

	internal virtual MethodInfo GetMethod(MethodInfo fromNoninstanciated)
	{
		throw new InvalidOperationException("can only be called in generic type");
	}

	internal virtual ConstructorInfo GetConstructor(ConstructorInfo fromNoninstanciated)
	{
		throw new InvalidOperationException("can only be called in generic type");
	}

	internal virtual FieldInfo GetField(FieldInfo fromNoninstanciated)
	{
		throw new InvalidOperationException("can only be called in generic type");
	}

	public static Type GetTypeFromHandle(RuntimeTypeHandle handle)
	{
		if (handle.Value == IntPtr.Zero)
		{
			return null;
		}
		return internal_from_handle(handle.Value);
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern Type internal_from_handle(IntPtr handle);
}
