using System.Reflection;

namespace System.Xml.Serialization;

internal class ConstantModel
{
	private FieldInfo fieldInfo;

	private long value;

	internal string Name => fieldInfo.Name;

	internal long Value => value;

	internal FieldInfo FieldInfo => fieldInfo;

	internal ConstantModel(FieldInfo fieldInfo, long value)
	{
		this.fieldInfo = fieldInfo;
		this.value = value;
	}
}
