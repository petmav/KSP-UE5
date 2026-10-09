using System.CodeDom.Compiler;
using System.Reflection;

namespace System.Xml.Serialization;

internal class MemberMapping : AccessorMapping
{
	private string name;

	private bool checkShouldPersist;

	private SpecifiedAccessor checkSpecified;

	private bool isReturnValue;

	private bool readOnly;

	private int sequenceId = -1;

	private MemberInfo memberInfo;

	private MemberInfo checkSpecifiedMemberInfo;

	private MethodInfo checkShouldPersistMethodInfo;

	internal bool CheckShouldPersist
	{
		get
		{
			return checkShouldPersist;
		}
		set
		{
			checkShouldPersist = value;
		}
	}

	internal SpecifiedAccessor CheckSpecified
	{
		get
		{
			return checkSpecified;
		}
		set
		{
			checkSpecified = value;
		}
	}

	internal string Name
	{
		get
		{
			if (name != null)
			{
				return name;
			}
			return string.Empty;
		}
		set
		{
			name = value;
		}
	}

	internal MemberInfo MemberInfo
	{
		get
		{
			return memberInfo;
		}
		set
		{
			memberInfo = value;
		}
	}

	internal MemberInfo CheckSpecifiedMemberInfo
	{
		get
		{
			return checkSpecifiedMemberInfo;
		}
		set
		{
			checkSpecifiedMemberInfo = value;
		}
	}

	internal MethodInfo CheckShouldPersistMethodInfo
	{
		get
		{
			return checkShouldPersistMethodInfo;
		}
		set
		{
			checkShouldPersistMethodInfo = value;
		}
	}

	internal bool IsReturnValue
	{
		get
		{
			return isReturnValue;
		}
		set
		{
			isReturnValue = value;
		}
	}

	internal bool ReadOnly
	{
		get
		{
			return readOnly;
		}
		set
		{
			readOnly = value;
		}
	}

	internal bool IsSequence => sequenceId >= 0;

	internal int SequenceId
	{
		get
		{
			return sequenceId;
		}
		set
		{
			sequenceId = value;
		}
	}

	internal MemberMapping()
	{
	}

	private MemberMapping(MemberMapping mapping)
		: base(mapping)
	{
		name = mapping.name;
		checkShouldPersist = mapping.checkShouldPersist;
		checkSpecified = mapping.checkSpecified;
		isReturnValue = mapping.isReturnValue;
		readOnly = mapping.readOnly;
		sequenceId = mapping.sequenceId;
		memberInfo = mapping.memberInfo;
		checkSpecifiedMemberInfo = mapping.checkSpecifiedMemberInfo;
		checkShouldPersistMethodInfo = mapping.checkShouldPersistMethodInfo;
	}

	private string GetNullableType(TypeDesc td)
	{
		if (td.IsMappedType || (!td.IsValueType && (base.Elements[0].IsSoap || td.ArrayElementTypeDesc == null)))
		{
			return td.FullName;
		}
		if (td.ArrayElementTypeDesc != null)
		{
			return GetNullableType(td.ArrayElementTypeDesc) + "[]";
		}
		return "System.Nullable`1[" + td.FullName + "]";
	}

	internal MemberMapping Clone()
	{
		return new MemberMapping(this);
	}

	internal string GetTypeName(CodeDomProvider codeProvider)
	{
		if (base.IsNeedNullable && codeProvider.Supports(GeneratorSupport.GenericTypeReference))
		{
			return GetNullableType(base.TypeDesc);
		}
		return base.TypeDesc.FullName;
	}
}
