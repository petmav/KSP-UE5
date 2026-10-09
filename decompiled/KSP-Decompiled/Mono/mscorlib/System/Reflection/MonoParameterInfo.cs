using System.Collections.Generic;
using System.Reflection.Emit;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace System.Reflection;

[Serializable]
[StructLayout(LayoutKind.Sequential)]
[ComDefaultInterface(typeof(_ParameterInfo))]
[ComVisible(true)]
[ClassInterface(ClassInterfaceType.None)]
internal class MonoParameterInfo : RuntimeParameterInfo
{
	public override object DefaultValue
	{
		get
		{
			if (ClassImpl == typeof(decimal))
			{
				DecimalConstantAttribute[] array = (DecimalConstantAttribute[])GetCustomAttributes(typeof(DecimalConstantAttribute), inherit: false);
				if (array.Length != 0)
				{
					return array[0].Value;
				}
			}
			else if (ClassImpl == typeof(DateTime))
			{
				DateTimeConstantAttribute[] array2 = (DateTimeConstantAttribute[])GetCustomAttributes(typeof(DateTimeConstantAttribute), inherit: false);
				if (array2.Length != 0)
				{
					return array2[0].Value;
				}
			}
			return DefaultValueImpl;
		}
	}

	public override object RawDefaultValue => DefaultValue;

	public override int MetadataToken
	{
		get
		{
			if (MemberImpl is PropertyInfo)
			{
				PropertyInfo propertyInfo = (PropertyInfo)MemberImpl;
				MethodInfo methodInfo = propertyInfo.GetGetMethod(nonPublic: true);
				if (methodInfo == null)
				{
					methodInfo = propertyInfo.GetSetMethod(nonPublic: true);
				}
				return methodInfo.GetParametersInternal()[PositionImpl].MetadataToken;
			}
			if (MemberImpl is MethodBase)
			{
				return GetMetadataToken();
			}
			throw new ArgumentException("Can't produce MetadataToken for member of type " + MemberImpl.GetType());
		}
	}

	public override bool HasDefaultValue
	{
		get
		{
			object defaultValue = DefaultValue;
			if (defaultValue == null)
			{
				return true;
			}
			if (defaultValue.GetType() == typeof(DBNull) || defaultValue.GetType() == typeof(Missing))
			{
				return false;
			}
			return true;
		}
	}

	internal MonoParameterInfo(ParameterBuilder pb, Type type, MemberInfo member, int position)
	{
		ClassImpl = type;
		MemberImpl = member;
		if (pb != null)
		{
			NameImpl = pb.Name;
			PositionImpl = pb.Position - 1;
			AttrsImpl = (ParameterAttributes)pb.Attributes;
		}
		else
		{
			NameImpl = null;
			PositionImpl = position - 1;
			AttrsImpl = ParameterAttributes.None;
		}
	}

	internal MonoParameterInfo(ParameterInfo pinfo, Type type, MemberInfo member, int position)
	{
		ClassImpl = type;
		MemberImpl = member;
		if (pinfo != null)
		{
			NameImpl = pinfo.Name;
			PositionImpl = pinfo.Position - 1;
			AttrsImpl = pinfo.Attributes;
		}
		else
		{
			NameImpl = null;
			PositionImpl = position - 1;
			AttrsImpl = ParameterAttributes.None;
		}
	}

	internal MonoParameterInfo(ParameterInfo pinfo, MemberInfo member)
	{
		ClassImpl = pinfo.ParameterType;
		MemberImpl = member;
		NameImpl = pinfo.Name;
		PositionImpl = pinfo.Position;
		AttrsImpl = pinfo.Attributes;
		DefaultValueImpl = pinfo.GetDefaultValueImpl();
	}

	internal MonoParameterInfo(Type type, MemberInfo member, MarshalAsAttribute marshalAs)
	{
		ClassImpl = type;
		MemberImpl = member;
		NameImpl = "";
		PositionImpl = -1;
		AttrsImpl = ParameterAttributes.Retval;
		base.marshalAs = marshalAs;
	}

	public override object[] GetCustomAttributes(bool inherit)
	{
		return MonoCustomAttrs.GetCustomAttributes(this, inherit);
	}

	public override object[] GetCustomAttributes(Type attributeType, bool inherit)
	{
		return MonoCustomAttrs.GetCustomAttributes(this, attributeType, inherit);
	}

	public override bool IsDefined(Type attributeType, bool inherit)
	{
		return MonoCustomAttrs.IsDefined(this, attributeType, inherit);
	}

	public override IList<CustomAttributeData> GetCustomAttributesData()
	{
		return CustomAttributeData.GetCustomAttributes(this);
	}

	public override Type[] GetOptionalCustomModifiers()
	{
		Type[] typeModifiers = GetTypeModifiers(optional: true);
		if (typeModifiers == null)
		{
			return Type.EmptyTypes;
		}
		return typeModifiers;
	}

	public override Type[] GetRequiredCustomModifiers()
	{
		Type[] typeModifiers = GetTypeModifiers(optional: false);
		if (typeModifiers == null)
		{
			return Type.EmptyTypes;
		}
		return typeModifiers;
	}
}
