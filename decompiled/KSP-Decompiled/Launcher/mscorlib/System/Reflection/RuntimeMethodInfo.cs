using System.Runtime.Serialization;
using System.Text;

namespace System.Reflection;

internal abstract class RuntimeMethodInfo : MethodInfo, ISerializable
{
	internal BindingFlags BindingFlags => BindingFlags.Default;

	public override Module Module => GetRuntimeModule();

	private RuntimeType ReflectedTypeInternal => (RuntimeType)ReflectedType;

	internal override string FormatNameAndSig(bool serialization)
	{
		StringBuilder stringBuilder = new StringBuilder(Name);
		TypeNameFormatFlags format = (serialization ? TypeNameFormatFlags.FormatSerialization : TypeNameFormatFlags.FormatBasic);
		if (IsGenericMethod)
		{
			stringBuilder.Append(RuntimeMethodHandle.ConstructInstantiation(this, format));
		}
		stringBuilder.Append("(");
		ParameterInfo.FormatParameters(stringBuilder, GetParametersNoCopy(), CallingConvention, serialization);
		stringBuilder.Append(")");
		return stringBuilder.ToString();
	}

	public override Delegate CreateDelegate(Type delegateType)
	{
		return Delegate.CreateDelegate(delegateType, this);
	}

	public override Delegate CreateDelegate(Type delegateType, object target)
	{
		return Delegate.CreateDelegate(delegateType, target, this);
	}

	public override string ToString()
	{
		return ReturnType.FormatTypeName() + " " + FormatNameAndSig(serialization: false);
	}

	internal RuntimeModule GetRuntimeModule()
	{
		return ((RuntimeType)DeclaringType).GetRuntimeModule();
	}

	public void GetObjectData(SerializationInfo info, StreamingContext context)
	{
		if (info == null)
		{
			throw new ArgumentNullException("info");
		}
		MemberInfoSerializationHolder.GetSerializationInfo(info, Name, ReflectedTypeInternal, ToString(), SerializationToString(), MemberTypes.Method, (IsGenericMethod & !IsGenericMethodDefinition) ? GetGenericArguments() : null);
	}

	internal string SerializationToString()
	{
		return ReturnType.FormatTypeName(serialization: true) + " " + FormatNameAndSig(serialization: true);
	}
}
