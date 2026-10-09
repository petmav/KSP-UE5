using System.Runtime.Serialization;
using System.Text;

namespace System.Reflection;

internal abstract class RuntimePropertyInfo : PropertyInfo, ISerializable
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

	public override string ToString()
	{
		return FormatNameAndSig(serialization: false);
	}

	private string FormatNameAndSig(bool serialization)
	{
		StringBuilder stringBuilder = new StringBuilder(PropertyType.FormatTypeName(serialization));
		stringBuilder.Append(" ");
		stringBuilder.Append(Name);
		ParameterInfo[] indexParameters = GetIndexParameters();
		if (indexParameters.Length != 0)
		{
			stringBuilder.Append(" [");
			ParameterInfo.FormatParameters(stringBuilder, indexParameters, (CallingConventions)0, serialization);
			stringBuilder.Append("]");
		}
		return stringBuilder.ToString();
	}

	public void GetObjectData(SerializationInfo info, StreamingContext context)
	{
		if (info == null)
		{
			throw new ArgumentNullException("info");
		}
		MemberInfoSerializationHolder.GetSerializationInfo(info, Name, ReflectedTypeInternal, ToString(), SerializationToString(), MemberTypes.Property, null);
	}

	internal string SerializationToString()
	{
		return FormatNameAndSig(serialization: true);
	}
}
