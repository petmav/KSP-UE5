using System.Diagnostics;
using System.Globalization;
using System.Reflection.Emit;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Security;
using System.Security.Permissions;
using System.Text;

namespace System.Reflection;

[Serializable]
[ClassInterface(ClassInterfaceType.None)]
[ComVisible(true)]
[ComDefaultInterface(typeof(_MethodBase))]
[PermissionSet(SecurityAction.InheritanceDemand, Name = "FullTrust")]
public abstract class MethodBase : MemberInfo, _MethodBase
{
	public virtual MethodImplAttributes MethodImplementationFlags => GetMethodImplementationFlags();

	public abstract RuntimeMethodHandle MethodHandle { get; }

	public abstract MethodAttributes Attributes { get; }

	public virtual CallingConventions CallingConvention => CallingConventions.Standard;

	public virtual bool IsGenericMethodDefinition => false;

	public virtual bool ContainsGenericParameters => false;

	public virtual bool IsGenericMethod => false;

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

	public bool IsPublic => (Attributes & MethodAttributes.MemberAccessMask) == MethodAttributes.Public;

	public bool IsPrivate => (Attributes & MethodAttributes.MemberAccessMask) == MethodAttributes.Private;

	public bool IsFamily => (Attributes & MethodAttributes.MemberAccessMask) == MethodAttributes.Family;

	public bool IsAssembly => (Attributes & MethodAttributes.MemberAccessMask) == MethodAttributes.Assembly;

	public bool IsFamilyAndAssembly => (Attributes & MethodAttributes.MemberAccessMask) == MethodAttributes.FamANDAssem;

	public bool IsFamilyOrAssembly => (Attributes & MethodAttributes.MemberAccessMask) == MethodAttributes.FamORAssem;

	public bool IsStatic => (Attributes & MethodAttributes.Static) != 0;

	public bool IsFinal => (Attributes & MethodAttributes.Final) != 0;

	public bool IsVirtual => (Attributes & MethodAttributes.Virtual) != 0;

	public bool IsHideBySig => (Attributes & MethodAttributes.HideBySig) != 0;

	public bool IsAbstract => (Attributes & MethodAttributes.Abstract) != 0;

	public bool IsSpecialName => (Attributes & MethodAttributes.SpecialName) != 0;

	[ComVisible(true)]
	public bool IsConstructor
	{
		get
		{
			if (this is ConstructorInfo && !IsStatic)
			{
				return (Attributes & MethodAttributes.RTSpecialName) == MethodAttributes.RTSpecialName;
			}
			return false;
		}
	}

	internal string FullName => $"{DeclaringType.FullName}.{FormatNameAndSig()}";

	bool _MethodBase.IsPublic => IsPublic;

	bool _MethodBase.IsPrivate => IsPrivate;

	bool _MethodBase.IsFamily => IsFamily;

	bool _MethodBase.IsAssembly => IsAssembly;

	bool _MethodBase.IsFamilyAndAssembly => IsFamilyAndAssembly;

	bool _MethodBase.IsFamilyOrAssembly => IsFamilyOrAssembly;

	bool _MethodBase.IsStatic => IsStatic;

	bool _MethodBase.IsFinal => IsFinal;

	bool _MethodBase.IsVirtual => IsVirtual;

	bool _MethodBase.IsHideBySig => IsHideBySig;

	bool _MethodBase.IsAbstract => IsAbstract;

	bool _MethodBase.IsSpecialName => IsSpecialName;

	bool _MethodBase.IsConstructor => IsConstructor;

	public static MethodBase GetMethodFromHandle(RuntimeMethodHandle handle)
	{
		if (handle.IsNullHandle())
		{
			throw new ArgumentException(Environment.GetResourceString("The handle is invalid."));
		}
		MethodBase methodFromHandleInternalType = GetMethodFromHandleInternalType(handle.Value, IntPtr.Zero);
		if (methodFromHandleInternalType == null)
		{
			throw new ArgumentException("The handle is invalid.");
		}
		Type declaringType = methodFromHandleInternalType.DeclaringType;
		if (declaringType != null && declaringType.IsGenericType)
		{
			throw new ArgumentException(string.Format(CultureInfo.CurrentCulture, Environment.GetResourceString("Cannot resolve method {0} because the declaring type of the method handle {1} is generic. Explicitly provide the declaring type to GetMethodFromHandle."), methodFromHandleInternalType, declaringType.GetGenericTypeDefinition()));
		}
		return methodFromHandleInternalType;
	}

	[ComVisible(false)]
	public static MethodBase GetMethodFromHandle(RuntimeMethodHandle handle, RuntimeTypeHandle declaringType)
	{
		if (handle.IsNullHandle())
		{
			throw new ArgumentException(Environment.GetResourceString("The handle is invalid."));
		}
		MethodBase methodFromHandleInternalType = GetMethodFromHandleInternalType(handle.Value, declaringType.Value);
		if (methodFromHandleInternalType == null)
		{
			throw new ArgumentException("The handle is invalid.");
		}
		return methodFromHandleInternalType;
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	public static extern MethodBase GetCurrentMethod();

	public static bool operator ==(MethodBase left, MethodBase right)
	{
		if ((object)left == right)
		{
			return true;
		}
		if ((object)left == null || (object)right == null)
		{
			return false;
		}
		MethodInfo methodInfo;
		MethodInfo methodInfo2;
		if ((methodInfo = left as MethodInfo) != null && (methodInfo2 = right as MethodInfo) != null)
		{
			return methodInfo == methodInfo2;
		}
		ConstructorInfo constructorInfo;
		ConstructorInfo constructorInfo2;
		if ((constructorInfo = left as ConstructorInfo) != null && (constructorInfo2 = right as ConstructorInfo) != null)
		{
			return constructorInfo == constructorInfo2;
		}
		return false;
	}

	public static bool operator !=(MethodBase left, MethodBase right)
	{
		return !(left == right);
	}

	public override bool Equals(object obj)
	{
		return base.Equals(obj);
	}

	public override int GetHashCode()
	{
		return base.GetHashCode();
	}

	[SecurityCritical]
	private IntPtr GetMethodDesc()
	{
		return MethodHandle.Value;
	}

	internal virtual ParameterInfo[] GetParametersNoCopy()
	{
		return GetParameters();
	}

	public abstract ParameterInfo[] GetParameters();

	public abstract MethodImplAttributes GetMethodImplementationFlags();

	public abstract object Invoke(object obj, BindingFlags invokeAttr, Binder binder, object[] parameters, CultureInfo culture);

	[ComVisible(true)]
	public virtual Type[] GetGenericArguments()
	{
		throw new NotSupportedException(Environment.GetResourceString("Derived classes must provide an implementation."));
	}

	[DebuggerStepThrough]
	[DebuggerHidden]
	public object Invoke(object obj, object[] parameters)
	{
		return Invoke(obj, BindingFlags.Default, null, parameters, null);
	}

	[SecuritySafeCritical]
	[ReflectionPermission(SecurityAction.Demand, Flags = ReflectionPermissionFlag.MemberAccess)]
	public virtual MethodBody GetMethodBody()
	{
		throw new InvalidOperationException();
	}

	internal static string ConstructParameters(Type[] parameterTypes, CallingConventions callingConvention, bool serialization)
	{
		StringBuilder stringBuilder = new StringBuilder();
		string value = "";
		foreach (Type obj in parameterTypes)
		{
			stringBuilder.Append(value);
			string text = obj.FormatTypeName(serialization);
			if (obj.IsByRef && !serialization)
			{
				stringBuilder.Append(text.TrimEnd('&'));
				stringBuilder.Append(" ByRef");
			}
			else
			{
				stringBuilder.Append(text);
			}
			value = ", ";
		}
		if ((callingConvention & CallingConventions.VarArgs) == CallingConventions.VarArgs)
		{
			stringBuilder.Append(value);
			stringBuilder.Append("...");
		}
		return stringBuilder.ToString();
	}

	internal string FormatNameAndSig()
	{
		return FormatNameAndSig(serialization: false);
	}

	internal virtual string FormatNameAndSig(bool serialization)
	{
		StringBuilder stringBuilder = new StringBuilder(Name);
		stringBuilder.Append("(");
		stringBuilder.Append(ConstructParameters(GetParameterTypes(), CallingConvention, serialization));
		stringBuilder.Append(")");
		return stringBuilder.ToString();
	}

	internal virtual Type[] GetParameterTypes()
	{
		ParameterInfo[] parametersNoCopy = GetParametersNoCopy();
		Type[] array = new Type[parametersNoCopy.Length];
		for (int i = 0; i < parametersNoCopy.Length; i++)
		{
			array[i] = parametersNoCopy[i].ParameterType;
		}
		return array;
	}

	Type _MethodBase.GetType()
	{
		return GetType();
	}

	void _MethodBase.GetTypeInfoCount(out uint pcTInfo)
	{
		throw new NotImplementedException();
	}

	void _MethodBase.GetTypeInfo(uint iTInfo, uint lcid, IntPtr ppTInfo)
	{
		throw new NotImplementedException();
	}

	void _MethodBase.GetIDsOfNames([In] ref Guid riid, IntPtr rgszNames, uint cNames, uint lcid, IntPtr rgDispId)
	{
		throw new NotImplementedException();
	}

	void _MethodBase.Invoke(uint dispIdMember, [In] ref Guid riid, uint lcid, short wFlags, IntPtr pDispParams, IntPtr pVarResult, IntPtr pExcepInfo, IntPtr puArgErr)
	{
		throw new NotImplementedException();
	}

	internal virtual ParameterInfo[] GetParametersInternal()
	{
		return GetParameters();
	}

	internal virtual int GetParametersCount()
	{
		return GetParametersInternal().Length;
	}

	internal virtual Type GetParameterType(int pos)
	{
		throw new NotImplementedException();
	}

	internal virtual int get_next_table_index(object obj, int table, bool inc)
	{
		if (this is MethodBuilder)
		{
			return ((MethodBuilder)this).get_next_table_index(obj, table, inc);
		}
		if (this is ConstructorBuilder)
		{
			return ((ConstructorBuilder)this).get_next_table_index(obj, table, inc);
		}
		throw new Exception("Method is not a builder method");
	}

	internal static MethodBase GetMethodFromHandleNoGenericCheck(RuntimeMethodHandle handle)
	{
		return GetMethodFromHandleInternalType_native(handle.Value, IntPtr.Zero, genericCheck: false);
	}

	internal static MethodBase GetMethodFromHandleNoGenericCheck(RuntimeMethodHandle handle, RuntimeTypeHandle reflectedType)
	{
		return GetMethodFromHandleInternalType_native(handle.Value, reflectedType.Value, genericCheck: false);
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	internal static extern MethodBody GetMethodBodyInternal(IntPtr handle);

	internal static MethodBody GetMethodBody(IntPtr handle)
	{
		return GetMethodBodyInternal(handle);
	}

	private static MethodBase GetMethodFromHandleInternalType(IntPtr method_handle, IntPtr type_handle)
	{
		return GetMethodFromHandleInternalType_native(method_handle, type_handle, genericCheck: true);
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	internal static extern MethodBase GetMethodFromHandleInternalType_native(IntPtr method_handle, IntPtr type_handle, bool genericCheck);
}
