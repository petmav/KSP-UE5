using System.Collections;
using System.Reflection;
using System.Security.Permissions;

namespace System.Xml.Serialization;

internal static class DynamicAssemblies
{
	private static ArrayList assembliesInConfig = new ArrayList();

	private static volatile Hashtable nameToAssemblyMap = new Hashtable();

	private static volatile Hashtable assemblyToNameMap = new Hashtable();

	private static Hashtable tableIsTypeDynamic = Hashtable.Synchronized(new Hashtable());

	private static volatile FileIOPermission fileIOPermission;

	private static FileIOPermission UnrestrictedFileIOPermission
	{
		get
		{
			if (fileIOPermission == null)
			{
				fileIOPermission = new FileIOPermission(PermissionState.Unrestricted);
			}
			return fileIOPermission;
		}
	}

	internal static bool IsTypeDynamic(Type type)
	{
		object obj = tableIsTypeDynamic[type];
		if (obj == null)
		{
			UnrestrictedFileIOPermission.Assert();
			Assembly assembly = type.Assembly;
			bool flag = assembly.IsDynamic || string.IsNullOrEmpty(assembly.Location);
			if (!flag)
			{
				if (type.IsArray)
				{
					flag = IsTypeDynamic(type.GetElementType());
				}
				else if (type.IsGenericType)
				{
					Type[] genericArguments = type.GetGenericArguments();
					if (genericArguments != null)
					{
						foreach (Type type2 in genericArguments)
						{
							if (!(type2 == null) && !type2.IsGenericParameter)
							{
								flag = IsTypeDynamic(type2);
								if (flag)
								{
									break;
								}
							}
						}
					}
				}
			}
			obj = (tableIsTypeDynamic[type] = flag);
		}
		return (bool)obj;
	}

	internal static bool IsTypeDynamic(Type[] arguments)
	{
		for (int i = 0; i < arguments.Length; i++)
		{
			if (IsTypeDynamic(arguments[i]))
			{
				return true;
			}
		}
		return false;
	}

	internal static void Add(Assembly a)
	{
		lock (nameToAssemblyMap)
		{
			if (assemblyToNameMap[a] == null)
			{
				Assembly assembly = nameToAssemblyMap[a.FullName] as Assembly;
				string text = null;
				if (assembly == null)
				{
					text = a.FullName;
				}
				else if (assembly != a)
				{
					text = a.FullName + ", " + nameToAssemblyMap.Count;
				}
				if (text != null)
				{
					nameToAssemblyMap.Add(text, a);
					assemblyToNameMap.Add(a, text);
				}
			}
		}
	}

	internal static Assembly Get(string fullName)
	{
		if (nameToAssemblyMap == null)
		{
			return null;
		}
		return (Assembly)nameToAssemblyMap[fullName];
	}

	internal static string GetName(Assembly a)
	{
		if (assemblyToNameMap == null)
		{
			return null;
		}
		return (string)assemblyToNameMap[a];
	}
}
