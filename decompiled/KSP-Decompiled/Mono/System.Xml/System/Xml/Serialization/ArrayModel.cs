namespace System.Xml.Serialization;

internal class ArrayModel : TypeModel
{
	internal TypeModel Element => base.ModelScope.GetTypeModel(TypeScope.GetArrayElementType(base.Type, null));

	internal ArrayModel(Type type, TypeDesc typeDesc, ModelScope scope)
		: base(type, typeDesc, scope)
	{
	}
}
