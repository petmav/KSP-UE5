using System.Xml.Schema;
using System.Xml.Serialization.Advanced;

namespace System.Xml.Serialization;

internal class TypeDesc
{
	private string name;

	private string fullName;

	private string cSharpName;

	private TypeDesc arrayElementTypeDesc;

	private TypeDesc arrayTypeDesc;

	private TypeDesc nullableTypeDesc;

	private TypeKind kind;

	private XmlSchemaType dataType;

	private Type type;

	private TypeDesc baseTypeDesc;

	private TypeFlags flags;

	private string formatterName;

	private bool isXsdType;

	private bool isMixed;

	private MappedTypeDesc extendedType;

	private int weight;

	private Exception exception;

	internal TypeFlags Flags => flags;

	internal bool IsXsdType => isXsdType;

	internal bool IsMappedType => extendedType != null;

	internal MappedTypeDesc ExtendedType => extendedType;

	internal string Name => name;

	internal string FullName => fullName;

	internal string CSharpName
	{
		get
		{
			if (cSharpName == null)
			{
				cSharpName = ((type == null) ? CodeIdentifier.GetCSharpName(fullName) : CodeIdentifier.GetCSharpName(type));
			}
			return cSharpName;
		}
	}

	internal XmlSchemaType DataType => dataType;

	internal Type Type => type;

	internal string FormatterName => formatterName;

	internal TypeKind Kind => kind;

	internal bool IsValueType => (flags & TypeFlags.Reference) == 0;

	internal bool CanBeAttributeValue => (flags & TypeFlags.CanBeAttributeValue) != 0;

	internal bool XmlEncodingNotRequired => (flags & TypeFlags.XmlEncodingNotRequired) != 0;

	internal bool CanBeElementValue => (flags & TypeFlags.CanBeElementValue) != 0;

	internal bool CanBeTextValue => (flags & TypeFlags.CanBeTextValue) != 0;

	internal bool IsMixed
	{
		get
		{
			if (!isMixed)
			{
				return CanBeTextValue;
			}
			return true;
		}
		set
		{
			isMixed = value;
		}
	}

	internal bool IsSpecial => (flags & TypeFlags.Special) != 0;

	internal bool IsAmbiguousDataType => (flags & TypeFlags.AmbiguousDataType) != 0;

	internal bool HasCustomFormatter => (flags & TypeFlags.HasCustomFormatter) != 0;

	internal bool HasDefaultSupport => (flags & TypeFlags.IgnoreDefault) == 0;

	internal bool HasIsEmpty => (flags & TypeFlags.HasIsEmpty) != 0;

	internal bool CollapseWhitespace => (flags & TypeFlags.CollapseWhitespace) != 0;

	internal bool HasDefaultConstructor => (flags & TypeFlags.HasDefaultConstructor) != 0;

	internal bool IsUnsupported => (flags & TypeFlags.Unsupported) != 0;

	internal bool IsGenericInterface => (flags & TypeFlags.GenericInterface) != 0;

	internal bool IsPrivateImplementation => (flags & TypeFlags.UsePrivateImplementation) != 0;

	internal bool CannotNew
	{
		get
		{
			if (HasDefaultConstructor)
			{
				return ConstructorInaccessible;
			}
			return true;
		}
	}

	internal bool IsAbstract => (flags & TypeFlags.Abstract) != 0;

	internal bool IsOptionalValue => (flags & TypeFlags.OptionalValue) != 0;

	internal bool UseReflection => (flags & TypeFlags.UseReflection) != 0;

	internal bool IsVoid => kind == TypeKind.Void;

	internal bool IsClass => kind == TypeKind.Class;

	internal bool IsStructLike
	{
		get
		{
			if (kind != TypeKind.Struct)
			{
				return kind == TypeKind.Class;
			}
			return true;
		}
	}

	internal bool IsArrayLike
	{
		get
		{
			if (kind != TypeKind.Array && kind != TypeKind.Collection)
			{
				return kind == TypeKind.Enumerable;
			}
			return true;
		}
	}

	internal bool IsCollection => kind == TypeKind.Collection;

	internal bool IsEnumerable => kind == TypeKind.Enumerable;

	internal bool IsArray => kind == TypeKind.Array;

	internal bool IsPrimitive => kind == TypeKind.Primitive;

	internal bool IsEnum => kind == TypeKind.Enum;

	internal bool IsNullable => !IsValueType;

	internal bool IsRoot => kind == TypeKind.Root;

	internal bool ConstructorInaccessible => (flags & TypeFlags.CtorInaccessible) != 0;

	internal Exception Exception
	{
		get
		{
			return exception;
		}
		set
		{
			exception = value;
		}
	}

	internal string ArrayLengthName
	{
		get
		{
			if (kind != TypeKind.Array)
			{
				return "Count";
			}
			return "Length";
		}
	}

	internal TypeDesc ArrayElementTypeDesc
	{
		get
		{
			return arrayElementTypeDesc;
		}
		set
		{
			arrayElementTypeDesc = value;
		}
	}

	internal int Weight => weight;

	internal TypeDesc BaseTypeDesc
	{
		get
		{
			return baseTypeDesc;
		}
		set
		{
			baseTypeDesc = value;
			weight = ((baseTypeDesc != null) ? (baseTypeDesc.Weight + 1) : 0);
		}
	}

	internal TypeDesc(string name, string fullName, XmlSchemaType dataType, TypeKind kind, TypeDesc baseTypeDesc, TypeFlags flags, string formatterName)
	{
		this.name = name.Replace('+', '.');
		this.fullName = fullName.Replace('+', '.');
		this.kind = kind;
		this.baseTypeDesc = baseTypeDesc;
		this.flags = flags;
		isXsdType = kind == TypeKind.Primitive;
		if (isXsdType)
		{
			weight = 1;
		}
		else if (kind == TypeKind.Enum)
		{
			weight = 2;
		}
		else if (this.kind == TypeKind.Root)
		{
			weight = -1;
		}
		else
		{
			weight = ((baseTypeDesc != null) ? (baseTypeDesc.Weight + 1) : 0);
		}
		this.dataType = dataType;
		this.formatterName = formatterName;
	}

	internal TypeDesc(string name, string fullName, XmlSchemaType dataType, TypeKind kind, TypeDesc baseTypeDesc, TypeFlags flags)
		: this(name, fullName, dataType, kind, baseTypeDesc, flags, null)
	{
	}

	internal TypeDesc(string name, string fullName, TypeKind kind, TypeDesc baseTypeDesc, TypeFlags flags)
		: this(name, fullName, null, kind, baseTypeDesc, flags, null)
	{
	}

	internal TypeDesc(Type type, bool isXsdType, XmlSchemaType dataType, string formatterName, TypeFlags flags)
		: this(type.Name, type.FullName, dataType, TypeKind.Primitive, null, flags, formatterName)
	{
		this.isXsdType = isXsdType;
		this.type = type;
	}

	internal TypeDesc(Type type, string name, string fullName, TypeKind kind, TypeDesc baseTypeDesc, TypeFlags flags, TypeDesc arrayElementTypeDesc)
		: this(name, fullName, null, kind, baseTypeDesc, flags, null)
	{
		this.arrayElementTypeDesc = arrayElementTypeDesc;
		this.type = type;
	}

	public override string ToString()
	{
		return fullName;
	}

	internal TypeDesc GetNullableTypeDesc(Type type)
	{
		if (IsOptionalValue)
		{
			return this;
		}
		if (nullableTypeDesc == null)
		{
			nullableTypeDesc = new TypeDesc("NullableOf" + name, "System.Nullable`1[" + fullName + "]", null, TypeKind.Struct, this, flags | TypeFlags.OptionalValue, formatterName);
			nullableTypeDesc.type = type;
		}
		return nullableTypeDesc;
	}

	internal void CheckSupported()
	{
		if (IsUnsupported)
		{
			if (Exception != null)
			{
				throw Exception;
			}
			throw new NotSupportedException(Res.GetString("{0} is an unsupported type. Please use [XmlIgnore] attribute to exclude members of this type from serialization graph.", FullName));
		}
		if (baseTypeDesc != null)
		{
			baseTypeDesc.CheckSupported();
		}
		if (arrayElementTypeDesc != null)
		{
			arrayElementTypeDesc.CheckSupported();
		}
	}

	internal void CheckNeedConstructor()
	{
		if (!IsValueType && !IsAbstract && !HasDefaultConstructor)
		{
			flags |= TypeFlags.Unsupported;
			exception = new InvalidOperationException(Res.GetString("{0} cannot be serialized because it does not have a parameterless constructor.", FullName));
		}
	}

	internal TypeDesc CreateArrayTypeDesc()
	{
		if (arrayTypeDesc == null)
		{
			arrayTypeDesc = new TypeDesc(null, name + "[]", fullName + "[]", TypeKind.Array, null, TypeFlags.Reference | (flags & TypeFlags.UseReflection), this);
		}
		return arrayTypeDesc;
	}

	internal TypeDesc CreateMappedTypeDesc(MappedTypeDesc extension)
	{
		return new TypeDesc(extension.Name, extension.Name, null, kind, baseTypeDesc, flags, null)
		{
			isXsdType = isXsdType,
			isMixed = isMixed,
			extendedType = extension,
			dataType = dataType
		};
	}

	internal bool IsDerivedFrom(TypeDesc baseTypeDesc)
	{
		for (TypeDesc typeDesc = this; typeDesc != null; typeDesc = typeDesc.BaseTypeDesc)
		{
			if (typeDesc == baseTypeDesc)
			{
				return true;
			}
		}
		return baseTypeDesc.IsRoot;
	}

	internal static TypeDesc FindCommonBaseTypeDesc(TypeDesc[] typeDescs)
	{
		if (typeDescs.Length == 0)
		{
			return null;
		}
		TypeDesc typeDesc = null;
		int num = int.MaxValue;
		for (int i = 0; i < typeDescs.Length; i++)
		{
			int num2 = typeDescs[i].Weight;
			if (num2 < num)
			{
				num = num2;
				typeDesc = typeDescs[i];
			}
		}
		while (typeDesc != null)
		{
			int j;
			for (j = 0; j < typeDescs.Length && typeDescs[j].IsDerivedFrom(typeDesc); j++)
			{
			}
			if (j == typeDescs.Length)
			{
				break;
			}
			typeDesc = typeDesc.BaseTypeDesc;
		}
		return typeDesc;
	}
}
