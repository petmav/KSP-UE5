using System.Collections.Generic;

namespace System.Reflection;

public static class RuntimeReflectionExtensions
{
	private const BindingFlags everything = BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public | BindingFlags.NonPublic;

	private static void CheckAndThrow(Type t)
	{
		if (t == null)
		{
			throw new ArgumentNullException("type");
		}
		if (!(t is RuntimeType))
		{
			throw new ArgumentException(Environment.GetResourceString("Type must be a runtime Type object."));
		}
	}

	private static void CheckAndThrow(MethodInfo m)
	{
		if (m == null)
		{
			throw new ArgumentNullException("method");
		}
		if (!(m is RuntimeMethodInfo))
		{
			throw new ArgumentException(Environment.GetResourceString("MethodInfo must be a runtime MethodInfo object."));
		}
	}

	public static IEnumerable<PropertyInfo> GetRuntimeProperties(this Type type)
	{
		CheckAndThrow(type);
		return type.GetProperties(BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public | BindingFlags.NonPublic);
	}

	public static IEnumerable<EventInfo> GetRuntimeEvents(this Type type)
	{
		CheckAndThrow(type);
		return type.GetEvents(BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public | BindingFlags.NonPublic);
	}

	public static IEnumerable<MethodInfo> GetRuntimeMethods(this Type type)
	{
		CheckAndThrow(type);
		return type.GetMethods(BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public | BindingFlags.NonPublic);
	}

	public static IEnumerable<FieldInfo> GetRuntimeFields(this Type type)
	{
		CheckAndThrow(type);
		return type.GetFields(BindingFlags.Instance | BindingFlags.Static | BindingFlags.Public | BindingFlags.NonPublic);
	}

	public static PropertyInfo GetRuntimeProperty(this Type type, string name)
	{
		CheckAndThrow(type);
		return type.GetProperty(name);
	}

	public static EventInfo GetRuntimeEvent(this Type type, string name)
	{
		CheckAndThrow(type);
		return type.GetEvent(name);
	}

	public static MethodInfo GetRuntimeMethod(this Type type, string name, Type[] parameters)
	{
		CheckAndThrow(type);
		return type.GetMethod(name, parameters);
	}

	public static FieldInfo GetRuntimeField(this Type type, string name)
	{
		CheckAndThrow(type);
		return type.GetField(name);
	}

	public static MethodInfo GetRuntimeBaseDefinition(this MethodInfo method)
	{
		CheckAndThrow(method);
		return method.GetBaseDefinition();
	}

	public static InterfaceMapping GetRuntimeInterfaceMap(this TypeInfo typeInfo, Type interfaceType)
	{
		if (typeInfo == null)
		{
			throw new ArgumentNullException("typeInfo");
		}
		if (!(typeInfo is RuntimeType))
		{
			throw new ArgumentException(Environment.GetResourceString("Type must be a runtime Type object."));
		}
		return typeInfo.GetInterfaceMap(interfaceType);
	}

	public static MethodInfo GetMethodInfo(this Delegate del)
	{
		if ((object)del == null)
		{
			throw new ArgumentNullException("del");
		}
		return del.Method;
	}
}
