using System.Collections;
using System.Reflection;

namespace System.Xml.Serialization;

internal class EnumModel : TypeModel
{
	private ConstantModel[] constants;

	internal ConstantModel[] Constants
	{
		get
		{
			if (constants == null)
			{
				ArrayList arrayList = new ArrayList();
				FieldInfo[] fields = base.Type.GetFields();
				foreach (FieldInfo fieldInfo in fields)
				{
					ConstantModel constantModel = GetConstantModel(fieldInfo);
					if (constantModel != null)
					{
						arrayList.Add(constantModel);
					}
				}
				constants = (ConstantModel[])arrayList.ToArray(typeof(ConstantModel));
			}
			return constants;
		}
	}

	internal EnumModel(Type type, TypeDesc typeDesc, ModelScope scope)
		: base(type, typeDesc, scope)
	{
	}

	private ConstantModel GetConstantModel(FieldInfo fieldInfo)
	{
		if (fieldInfo.IsSpecialName)
		{
			return null;
		}
		return new ConstantModel(fieldInfo, ((IConvertible)fieldInfo.GetValue(null)).ToInt64(null));
	}
}
