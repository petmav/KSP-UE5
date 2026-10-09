using System.Reflection;
using System.Security;
using System.Security.Permissions;

namespace System.Xml;

internal static class BinaryCompatibility
{
	private static bool _targetsAtLeast_Desktop_V4_5_2 = RunningOnCheck("TargetsAtLeast_Desktop_V4_5_2");

	internal static bool TargetsAtLeast_Desktop_V4_5_2 => _targetsAtLeast_Desktop_V4_5_2;

	[SecuritySafeCritical]
	[ReflectionPermission(SecurityAction.Assert, Unrestricted = true)]
	private static bool RunningOnCheck(string propertyName)
	{
		Type type;
		try
		{
			type = typeof(object).GetTypeInfo().Assembly.GetType("System.Runtime.Versioning.BinaryCompatibility", throwOnError: false);
		}
		catch (TypeLoadException)
		{
			return false;
		}
		if (type == null)
		{
			return false;
		}
		PropertyInfo property = type.GetProperty(propertyName, BindingFlags.Static | BindingFlags.Public | BindingFlags.NonPublic);
		if (property == null)
		{
			return false;
		}
		return (bool)property.GetValue(null);
	}
}
