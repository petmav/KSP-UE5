namespace System.Xml.Serialization;

[AttributeUsage(AttributeTargets.Class | AttributeTargets.Struct | AttributeTargets.Enum | AttributeTargets.Interface, AllowMultiple = false)]
public sealed class XmlSerializerAssemblyAttribute : Attribute
{
	private string assemblyName;

	private string codeBase;

	public string CodeBase
	{
		get
		{
			return codeBase;
		}
		set
		{
			codeBase = value;
		}
	}

	public string AssemblyName
	{
		get
		{
			return assemblyName;
		}
		set
		{
			assemblyName = value;
		}
	}

	public XmlSerializerAssemblyAttribute()
		: this(null, null)
	{
	}

	public XmlSerializerAssemblyAttribute(string assemblyName)
		: this(assemblyName, null)
	{
	}

	public XmlSerializerAssemblyAttribute(string assemblyName, string codeBase)
	{
		this.assemblyName = assemblyName;
		this.codeBase = codeBase;
	}
}
