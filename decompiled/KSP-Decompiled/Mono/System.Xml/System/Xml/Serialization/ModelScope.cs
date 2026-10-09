using System.Collections;

namespace System.Xml.Serialization;

internal class ModelScope
{
	private TypeScope typeScope;

	private Hashtable models = new Hashtable();

	private Hashtable arrayModels = new Hashtable();

	internal TypeScope TypeScope => typeScope;

	internal ModelScope(TypeScope typeScope)
	{
		this.typeScope = typeScope;
	}

	internal TypeModel GetTypeModel(Type type)
	{
		return GetTypeModel(type, directReference: true);
	}

	internal TypeModel GetTypeModel(Type type, bool directReference)
	{
		TypeModel typeModel = (TypeModel)models[type];
		if (typeModel != null)
		{
			return typeModel;
		}
		TypeDesc typeDesc = typeScope.GetTypeDesc(type, null, directReference);
		switch (typeDesc.Kind)
		{
		case TypeKind.Enum:
			typeModel = new EnumModel(type, typeDesc, this);
			break;
		case TypeKind.Primitive:
			typeModel = new PrimitiveModel(type, typeDesc, this);
			break;
		case TypeKind.Array:
		case TypeKind.Collection:
		case TypeKind.Enumerable:
			typeModel = new ArrayModel(type, typeDesc, this);
			break;
		case TypeKind.Root:
		case TypeKind.Struct:
		case TypeKind.Class:
			typeModel = new StructModel(type, typeDesc, this);
			break;
		default:
			if (!typeDesc.IsSpecial)
			{
				throw new NotSupportedException(Res.GetString("The type {0} may not be serialized.", type.FullName));
			}
			typeModel = new SpecialModel(type, typeDesc, this);
			break;
		}
		models.Add(type, typeModel);
		return typeModel;
	}

	internal ArrayModel GetArrayModel(Type type)
	{
		TypeModel typeModel = (TypeModel)arrayModels[type];
		if (typeModel == null)
		{
			typeModel = GetTypeModel(type);
			if (!(typeModel is ArrayModel))
			{
				TypeDesc arrayTypeDesc = typeScope.GetArrayTypeDesc(type);
				typeModel = new ArrayModel(type, arrayTypeDesc, this);
			}
			arrayModels.Add(type, typeModel);
		}
		return (ArrayModel)typeModel;
	}
}
