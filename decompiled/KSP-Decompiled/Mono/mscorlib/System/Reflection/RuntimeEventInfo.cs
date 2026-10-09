using System.Runtime.Serialization;

namespace System.Reflection;

internal abstract class RuntimeEventInfo : EventInfo, ISerializable
{
	internal BindingFlags BindingFlags => BindingFlags.Default;

	public override Module Module => GetRuntimeModule();

	private RuntimeType ReflectedTypeInternal => (RuntimeType)ReflectedType;

	internal RuntimeType GetDeclaringTypeInternal()
	{
		return (RuntimeType)DeclaringType;
	}

	internal RuntimeModule GetRuntimeModule()
	{
		return GetDeclaringTypeInternal().GetRuntimeModule();
	}

	public void GetObjectData(SerializationInfo info, StreamingContext context)
	{
		if (info == null)
		{
			throw new ArgumentNullException("info");
		}
		MemberInfoSerializationHolder.GetSerializationInfo(info, Name, ReflectedTypeInternal, null, MemberTypes.Event);
	}
}
