using System.Collections.Generic;
using System.Reflection.Emit;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Runtime.Serialization;
using System.Security;
using System.Text;

namespace System.Reflection;

[Serializable]
[StructLayout(LayoutKind.Sequential)]
[ComVisible(true)]
[ClassInterface(ClassInterfaceType.None)]
[ComDefaultInterface(typeof(_ParameterInfo))]
public class ParameterInfo : ICustomAttributeProvider, _ParameterInfo, IObjectReference
{
	protected Type ClassImpl;

	protected object DefaultValueImpl;

	protected MemberInfo MemberImpl;

	protected string NameImpl;

	protected int PositionImpl;

	protected ParameterAttributes AttrsImpl;

	internal MarshalAsAttribute marshalAs;

	public virtual Type ParameterType => ClassImpl;

	public virtual ParameterAttributes Attributes => AttrsImpl;

	public bool IsIn => (Attributes & ParameterAttributes.In) != 0;

	public bool IsLcid => (Attributes & ParameterAttributes.Lcid) != 0;

	public bool IsOptional => (Attributes & ParameterAttributes.Optional) != 0;

	public bool IsOut => (Attributes & ParameterAttributes.Out) != 0;

	public bool IsRetval => (Attributes & ParameterAttributes.Retval) != 0;

	public virtual MemberInfo Member => MemberImpl;

	public virtual string Name => NameImpl;

	public virtual int Position => PositionImpl;

	public virtual IEnumerable<CustomAttributeData> CustomAttributes => GetCustomAttributesData();

	public virtual bool HasDefaultValue
	{
		get
		{
			throw new NotImplementedException();
		}
	}

	public virtual object DefaultValue
	{
		get
		{
			throw new NotImplementedException();
		}
	}

	public virtual object RawDefaultValue
	{
		get
		{
			throw new NotImplementedException();
		}
	}

	public virtual int MetadataToken => 134217728;

	protected ParameterInfo()
	{
	}

	public override string ToString()
	{
		Type type = ClassImpl;
		while (type.HasElementType)
		{
			type = type.GetElementType();
		}
		string text = ((type.IsPrimitive || ClassImpl == typeof(void) || ClassImpl.Namespace == MemberImpl.DeclaringType.Namespace) ? ClassImpl.Name : ClassImpl.FullName);
		if (!IsRetval)
		{
			text += " ";
			text += NameImpl;
		}
		return text;
	}

	internal static void FormatParameters(StringBuilder sb, ParameterInfo[] p, CallingConventions callingConvention, bool serialization)
	{
		for (int i = 0; i < p.Length; i++)
		{
			if (i > 0)
			{
				sb.Append(", ");
			}
			Type parameterType = p[i].ParameterType;
			string text = parameterType.FormatTypeName(serialization);
			if (parameterType.IsByRef && !serialization)
			{
				sb.Append(text.TrimEnd('&'));
				sb.Append(" ByRef");
			}
			else
			{
				sb.Append(text);
			}
		}
		if ((callingConvention & CallingConventions.VarArgs) != 0)
		{
			if (p.Length != 0)
			{
				sb.Append(", ");
			}
			sb.Append("...");
		}
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	internal extern int GetMetadataToken();

	internal object[] GetPseudoCustomAttributes()
	{
		int num = 0;
		if (IsIn)
		{
			num++;
		}
		if (IsOut)
		{
			num++;
		}
		if (IsOptional)
		{
			num++;
		}
		if (marshalAs != null)
		{
			num++;
		}
		if (num == 0)
		{
			return null;
		}
		object[] array = new object[num];
		num = 0;
		if (IsIn)
		{
			array[num++] = new InAttribute();
		}
		if (IsOptional)
		{
			array[num++] = new OptionalAttribute();
		}
		if (IsOut)
		{
			array[num++] = new OutAttribute();
		}
		if (marshalAs != null)
		{
			array[num++] = marshalAs.Copy();
		}
		return array;
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	internal extern Type[] GetTypeModifiers(bool optional);

	internal object GetDefaultValueImpl()
	{
		return DefaultValueImpl;
	}

	void _ParameterInfo.GetIDsOfNames([In] ref Guid riid, IntPtr rgszNames, uint cNames, uint lcid, IntPtr rgDispId)
	{
		throw new NotImplementedException();
	}

	void _ParameterInfo.GetTypeInfo(uint iTInfo, uint lcid, IntPtr ppTInfo)
	{
		throw new NotImplementedException();
	}

	void _ParameterInfo.GetTypeInfoCount(out uint pcTInfo)
	{
		throw new NotImplementedException();
	}

	void _ParameterInfo.Invoke(uint dispIdMember, [In] ref Guid riid, uint lcid, short wFlags, IntPtr pDispParams, IntPtr pVarResult, IntPtr pExcepInfo, IntPtr puArgErr)
	{
		throw new NotImplementedException();
	}

	public virtual object[] GetCustomAttributes(bool inherit)
	{
		return new object[0];
	}

	public virtual object[] GetCustomAttributes(Type attributeType, bool inherit)
	{
		return new object[0];
	}

	[SecurityCritical]
	public object GetRealObject(StreamingContext context)
	{
		throw new NotImplementedException();
	}

	public virtual bool IsDefined(Type attributeType, bool inherit)
	{
		return false;
	}

	public virtual Type[] GetRequiredCustomModifiers()
	{
		return new Type[0];
	}

	public virtual Type[] GetOptionalCustomModifiers()
	{
		return new Type[0];
	}

	public virtual IList<CustomAttributeData> GetCustomAttributesData()
	{
		throw new NotImplementedException();
	}

	internal static ParameterInfo New(ParameterBuilder pb, Type type, MemberInfo member, int position)
	{
		return new MonoParameterInfo(pb, type, member, position);
	}

	internal static ParameterInfo New(ParameterInfo pinfo, Type type, MemberInfo member, int position)
	{
		return new MonoParameterInfo(pinfo, type, member, position);
	}

	internal static ParameterInfo New(ParameterInfo pinfo, MemberInfo member)
	{
		return new MonoParameterInfo(pinfo, member);
	}

	internal static ParameterInfo New(Type type, MemberInfo member, MarshalAsAttribute marshalAs)
	{
		return new MonoParameterInfo(type, member, marshalAs);
	}
}
