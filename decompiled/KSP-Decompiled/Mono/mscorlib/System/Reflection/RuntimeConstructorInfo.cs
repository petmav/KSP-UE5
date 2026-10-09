using System.Runtime.Serialization;

namespace System.Reflection;

internal abstract class RuntimeConstructorInfo : ConstructorInfo, ISerializable
{
	public override Module Module => GetRuntimeModule();

	internal BindingFlags BindingFlags => BindingFlags.Default;

	private RuntimeType ReflectedTypeInternal => (RuntimeType)ReflectedType;

	internal RuntimeModule GetRuntimeModule()
	{
		return RuntimeTypeHandle.GetModule((RuntimeType)DeclaringType);
	}

	public void GetObjectData(SerializationInfo info, StreamingContext context)
	{
		if (info == null)
		{
			throw new ArgumentNullException("info");
		}
		MemberInfoSerializationHolder.GetSerializationInfo(info, Name, ReflectedTypeInternal, ToString(), SerializationToString(), MemberTypes.Constructor, null);
	}

	internal string SerializationToString()
	{
		return FormatNameAndSig(serialization: true);
	}

	internal void SerializationInvoke(object target, SerializationInfo info, StreamingContext context)
	{
		Invoke(target, new object[2] { info, context });
	}
}
