using System.CodeDom.Compiler;
using System.Configuration;
using System.Xml.Serialization.Configuration;

namespace System.Xml.Serialization;

internal sealed class XmlSerializerCompilerParameters
{
	private bool needTempDirAccess;

	private CompilerParameters parameters;

	internal bool IsNeedTempDirAccess => needTempDirAccess;

	internal CompilerParameters CodeDomParameters => parameters;

	private XmlSerializerCompilerParameters(CompilerParameters parameters, bool needTempDirAccess)
	{
		this.needTempDirAccess = needTempDirAccess;
		this.parameters = parameters;
	}

	internal static XmlSerializerCompilerParameters Create(string location)
	{
		CompilerParameters obj = new CompilerParameters
		{
			GenerateInMemory = true
		};
		if (string.IsNullOrEmpty(location))
		{
			location = ((!(ConfigurationManager.GetSection(ConfigurationStrings.XmlSerializerSectionPath) is XmlSerializerSection xmlSerializerSection)) ? location : xmlSerializerSection.TempFilesLocation);
			if (!string.IsNullOrEmpty(location))
			{
				location = location.Trim();
			}
		}
		obj.TempFiles = new TempFileCollection(location);
		return new XmlSerializerCompilerParameters(obj, string.IsNullOrEmpty(location));
	}

	internal static XmlSerializerCompilerParameters Create(CompilerParameters parameters, bool needTempDirAccess)
	{
		return new XmlSerializerCompilerParameters(parameters, needTempDirAccess);
	}
}
