using System.Globalization;
using System.Runtime.InteropServices;
using Unity;

namespace System.Reflection.Emit;

public abstract class GenericTypeParameterBuilder : TypeInfo
{
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

	public override Type UnderlyingSystemType
	{
		get
		{
			ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public override Type GetElementType()
	{
		throw new PlatformNotSupportedException();
	}

	public void SetBaseTypeConstraint(Type baseTypeConstraint)
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

	public void SetGenericParameterAttributes(GenericParameterAttributes genericParameterAttributes)
	{
		throw new PlatformNotSupportedException();
	}

	public void SetInterfaceConstraints(Type[] interfaceConstraints)
	{
		throw new PlatformNotSupportedException();
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
}
