using System.Reflection;

namespace System.Xml.Serialization;

internal class FieldModel
{
	private SpecifiedAccessor checkSpecified;

	private MemberInfo memberInfo;

	private MemberInfo checkSpecifiedMemberInfo;

	private MethodInfo checkShouldPersistMethodInfo;

	private bool checkShouldPersist;

	private bool readOnly;

	private bool isProperty;

	private Type fieldType;

	private string name;

	private TypeDesc fieldTypeDesc;

	internal string Name => name;

	internal Type FieldType => fieldType;

	internal TypeDesc FieldTypeDesc => fieldTypeDesc;

	internal bool CheckShouldPersist => checkShouldPersist;

	internal SpecifiedAccessor CheckSpecified => checkSpecified;

	internal MemberInfo MemberInfo => memberInfo;

	internal MemberInfo CheckSpecifiedMemberInfo => checkSpecifiedMemberInfo;

	internal MethodInfo CheckShouldPersistMethodInfo => checkShouldPersistMethodInfo;

	internal bool ReadOnly => readOnly;

	internal bool IsProperty => isProperty;

	internal FieldModel(string name, Type fieldType, TypeDesc fieldTypeDesc, bool checkSpecified, bool checkShouldPersist)
		: this(name, fieldType, fieldTypeDesc, checkSpecified, checkShouldPersist, readOnly: false)
	{
	}

	internal FieldModel(string name, Type fieldType, TypeDesc fieldTypeDesc, bool checkSpecified, bool checkShouldPersist, bool readOnly)
	{
		this.fieldTypeDesc = fieldTypeDesc;
		this.name = name;
		this.fieldType = fieldType;
		this.checkSpecified = (checkSpecified ? SpecifiedAccessor.ReadWrite : SpecifiedAccessor.None);
		this.checkShouldPersist = checkShouldPersist;
		this.readOnly = readOnly;
	}

	internal FieldModel(MemberInfo memberInfo, Type fieldType, TypeDesc fieldTypeDesc)
	{
		name = memberInfo.Name;
		this.fieldType = fieldType;
		this.fieldTypeDesc = fieldTypeDesc;
		this.memberInfo = memberInfo;
		checkShouldPersistMethodInfo = memberInfo.DeclaringType.GetMethod("ShouldSerialize" + memberInfo.Name, new Type[0]);
		checkShouldPersist = checkShouldPersistMethodInfo != null;
		FieldInfo field = memberInfo.DeclaringType.GetField(memberInfo.Name + "Specified");
		if (field != null)
		{
			if (field.FieldType != typeof(bool))
			{
				throw new InvalidOperationException(Res.GetString("Member '{0}' of type {1} cannot be serialized.  Members with names ending on 'Specified' suffix have special meaning to the XmlSerializer: they control serialization of optional ValueType members and have to be of type {2}.", field.Name, field.FieldType.FullName, typeof(bool).FullName));
			}
			checkSpecified = (field.IsInitOnly ? SpecifiedAccessor.ReadOnly : SpecifiedAccessor.ReadWrite);
			checkSpecifiedMemberInfo = field;
		}
		else
		{
			PropertyInfo property = memberInfo.DeclaringType.GetProperty(memberInfo.Name + "Specified");
			if (property != null)
			{
				if (StructModel.CheckPropertyRead(property))
				{
					checkSpecified = ((!property.CanWrite) ? SpecifiedAccessor.ReadOnly : SpecifiedAccessor.ReadWrite);
					checkSpecifiedMemberInfo = property;
				}
				if (checkSpecified != 0 && property.PropertyType != typeof(bool))
				{
					throw new InvalidOperationException(Res.GetString("Member '{0}' of type {1} cannot be serialized.  Members with names ending on 'Specified' suffix have special meaning to the XmlSerializer: they control serialization of optional ValueType members and have to be of type {2}.", property.Name, property.PropertyType.FullName, typeof(bool).FullName));
				}
			}
		}
		if (memberInfo is PropertyInfo)
		{
			readOnly = !((PropertyInfo)memberInfo).CanWrite;
			isProperty = true;
		}
		else if (memberInfo is FieldInfo)
		{
			readOnly = ((FieldInfo)memberInfo).IsInitOnly;
		}
	}
}
