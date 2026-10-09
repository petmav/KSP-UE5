using System.Globalization;
using System.Runtime.InteropServices;
using Unity;

namespace System.Reflection.Emit;

public abstract class EnumBuilder : TypeInfo, _EnumBuilder
{
	public FieldBuilder UnderlyingField
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

	public TypeInfo CreateTypeInfo()
	{
		throw new PlatformNotSupportedException();
	}

	public FieldBuilder DefineLiteral(string literalName, object literalValue)
	{
		throw new PlatformNotSupportedException();
	}

	public override Type GetElementType()
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

	public Type CreateType()
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

	public override object[] GetCustomAttributes(bool inherit)
	{
		ThrowStub.ThrowNotSupportedException();
		return null;
	}

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

	void _EnumBuilder.GetIDsOfNames([In] ref Guid riid, IntPtr rgszNames, uint cNames, uint lcid, IntPtr rgDispId)
	{
		ThrowStub.ThrowNotSupportedException();
	}

	void _EnumBuilder.GetTypeInfo(uint iTInfo, uint lcid, IntPtr ppTInfo)
	{
		ThrowStub.ThrowNotSupportedException();
	}

	void _EnumBuilder.GetTypeInfoCount(out uint pcTInfo)
	{
		ThrowStub.ThrowNotSupportedException();
	}

	void _EnumBuilder.Invoke(uint dispIdMember, [In] ref Guid riid, uint lcid, short wFlags, IntPtr pDispParams, IntPtr pVarResult, IntPtr pExcepInfo, IntPtr puArgErr)
	{
		ThrowStub.ThrowNotSupportedException();
	}
}
