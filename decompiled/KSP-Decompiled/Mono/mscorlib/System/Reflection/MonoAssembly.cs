using System.Collections.Generic;
using System.Globalization;
using System.Runtime.InteropServices;

namespace System.Reflection;

[Serializable]
[ComVisible(true)]
[ComDefaultInterface(typeof(_Assembly))]
[ClassInterface(ClassInterfaceType.None)]
internal class MonoAssembly : RuntimeAssembly
{
	[ComVisible(false)]
	public override Module ManifestModule => GetManifestModule();

	public override bool GlobalAssemblyCache => get_global_assembly_cache();

	public override Type GetType(string name, bool throwOnError, bool ignoreCase)
	{
		if (name == null)
		{
			throw new ArgumentNullException(name);
		}
		if (name.Length == 0)
		{
			throw new ArgumentException("name", "Name cannot be empty");
		}
		return InternalGetType(null, name, throwOnError, ignoreCase);
	}

	public override Module GetModule(string name)
	{
		if (name == null)
		{
			throw new ArgumentNullException("name");
		}
		if (name.Length == 0)
		{
			throw new ArgumentException("Name can't be empty");
		}
		Module[] modules = GetModules(getResourceModules: true);
		foreach (Module module in modules)
		{
			if (module.ScopeName == name)
			{
				return module;
			}
		}
		return null;
	}

	public override AssemblyName[] GetReferencedAssemblies()
	{
		return Assembly.GetReferencedAssemblies(this);
	}

	public override Module[] GetModules(bool getResourceModules)
	{
		Module[] modulesInternal = GetModulesInternal();
		if (!getResourceModules)
		{
			List<Module> list = new List<Module>(modulesInternal.Length);
			Module[] array = modulesInternal;
			foreach (Module module in array)
			{
				if (!module.IsResource())
				{
					list.Add(module);
				}
			}
			return list.ToArray();
		}
		return modulesInternal;
	}

	[MonoTODO("Always returns the same as GetModules")]
	public override Module[] GetLoadedModules(bool getResourceModules)
	{
		return GetModules(getResourceModules);
	}

	public override Assembly GetSatelliteAssembly(CultureInfo culture)
	{
		return GetSatelliteAssembly(culture, null, throwOnError: true);
	}

	public override Assembly GetSatelliteAssembly(CultureInfo culture, Version version)
	{
		return GetSatelliteAssembly(culture, version, throwOnError: true);
	}
}
