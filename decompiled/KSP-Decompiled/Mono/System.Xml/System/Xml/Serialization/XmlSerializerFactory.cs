using System.Security.Permissions;
using System.Security.Policy;

namespace System.Xml.Serialization;

public class XmlSerializerFactory
{
	private static TempAssemblyCache cache = new TempAssemblyCache();

	public XmlSerializer CreateSerializer(Type type, XmlAttributeOverrides overrides, Type[] extraTypes, XmlRootAttribute root, string defaultNamespace)
	{
		return CreateSerializer(type, overrides, extraTypes, root, defaultNamespace, null);
	}

	public XmlSerializer CreateSerializer(Type type, XmlRootAttribute root)
	{
		return CreateSerializer(type, null, new Type[0], root, null, null);
	}

	public XmlSerializer CreateSerializer(Type type, Type[] extraTypes)
	{
		return CreateSerializer(type, null, extraTypes, null, null, null);
	}

	public XmlSerializer CreateSerializer(Type type, XmlAttributeOverrides overrides)
	{
		return CreateSerializer(type, overrides, new Type[0], null, null, null);
	}

	public XmlSerializer CreateSerializer(XmlTypeMapping xmlTypeMapping)
	{
		return (XmlSerializer)XmlSerializer.GenerateTempAssembly(xmlTypeMapping).Contract.TypedSerializers[xmlTypeMapping.Key];
	}

	public XmlSerializer CreateSerializer(Type type)
	{
		return CreateSerializer(type, (string)null);
	}

	public XmlSerializer CreateSerializer(Type type, string defaultNamespace)
	{
		if (type == null)
		{
			throw new ArgumentNullException("type");
		}
		TempAssembly tempAssembly = cache[defaultNamespace, type];
		XmlTypeMapping xmlTypeMapping = null;
		if (tempAssembly == null)
		{
			lock (cache)
			{
				tempAssembly = cache[defaultNamespace, type];
				if (tempAssembly == null)
				{
					if (TempAssembly.LoadGeneratedAssembly(type, defaultNamespace, out var contract) == null)
					{
						xmlTypeMapping = new XmlReflectionImporter(defaultNamespace).ImportTypeMapping(type, null, defaultNamespace);
						tempAssembly = XmlSerializer.GenerateTempAssembly(xmlTypeMapping, type, defaultNamespace);
					}
					else
					{
						tempAssembly = new TempAssembly(contract);
					}
					cache.Add(defaultNamespace, type, tempAssembly);
				}
			}
		}
		if (xmlTypeMapping == null)
		{
			xmlTypeMapping = XmlReflectionImporter.GetTopLevelMapping(type, defaultNamespace);
		}
		return tempAssembly.Contract.GetSerializer(type);
	}

	public XmlSerializer CreateSerializer(Type type, XmlAttributeOverrides overrides, Type[] extraTypes, XmlRootAttribute root, string defaultNamespace, string location)
	{
		return CreateSerializer(type, overrides, extraTypes, root, defaultNamespace, location, null);
	}

	[Obsolete("This method is obsolete and will be removed in a future release of the .NET Framework. Please use an overload of CreateSerializer which does not take an Evidence parameter. See http://go2.microsoft.com/fwlink/?LinkId=131738 for more information.")]
	public XmlSerializer CreateSerializer(Type type, XmlAttributeOverrides overrides, Type[] extraTypes, XmlRootAttribute root, string defaultNamespace, string location, Evidence evidence)
	{
		if (type == null)
		{
			throw new ArgumentNullException("type");
		}
		if (location != null || evidence != null)
		{
			DemandForUserLocationOrEvidence();
		}
		XmlReflectionImporter xmlReflectionImporter = new XmlReflectionImporter(overrides, defaultNamespace);
		for (int i = 0; i < extraTypes.Length; i++)
		{
			xmlReflectionImporter.IncludeType(extraTypes[i]);
		}
		XmlTypeMapping xmlTypeMapping = xmlReflectionImporter.ImportTypeMapping(type, root, defaultNamespace);
		return (XmlSerializer)XmlSerializer.GenerateTempAssembly(xmlTypeMapping, type, defaultNamespace, location, evidence).Contract.TypedSerializers[xmlTypeMapping.Key];
	}

	[PermissionSet(SecurityAction.Demand, Name = "FullTrust")]
	private void DemandForUserLocationOrEvidence()
	{
	}
}
