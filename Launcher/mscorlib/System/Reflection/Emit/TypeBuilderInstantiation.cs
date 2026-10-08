namespace System.Reflection.Emit;

internal abstract class TypeBuilderInstantiation : TypeInfo
{
	internal static Type MakeGenericType(Type type, Type[] typeArguments)
	{
		throw new NotSupportedException("User types are not supported under full aot");
	}
}
