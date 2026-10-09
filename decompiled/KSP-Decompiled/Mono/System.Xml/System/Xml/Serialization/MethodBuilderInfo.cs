using System.Reflection;
using System.Reflection.Emit;

namespace System.Xml.Serialization;

internal class MethodBuilderInfo
{
	public readonly MethodBuilder MethodBuilder;

	public readonly Type[] ParameterTypes;

	public MethodBuilderInfo(MethodBuilder methodBuilder, Type[] parameterTypes)
	{
		MethodBuilder = methodBuilder;
		ParameterTypes = parameterTypes;
	}

	public void Validate(Type returnType, Type[] parameterTypes, MethodAttributes attributes)
	{
	}
}
