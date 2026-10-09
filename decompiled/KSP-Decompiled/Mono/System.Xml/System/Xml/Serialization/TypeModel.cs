namespace System.Xml.Serialization;

internal abstract class TypeModel
{
	private TypeDesc typeDesc;

	private Type type;

	private ModelScope scope;

	internal Type Type => type;

	internal ModelScope ModelScope => scope;

	internal TypeDesc TypeDesc => typeDesc;

	protected TypeModel(Type type, TypeDesc typeDesc, ModelScope scope)
	{
		this.scope = scope;
		this.type = type;
		this.typeDesc = typeDesc;
	}
}
