using System;

namespace Mono.Cecil
{
	public class FieldReference : MemberReference
	{
		private TypeReference field_type;

		public TypeReference FieldType
		{
			get
			{
				return field_type;
			}
			set
			{
				field_type = value;
			}
		}

		public override string FullName => field_type.FullName + " " + MemberFullName();

		public override bool ContainsGenericParameter
		{
			get
			{
				if (!field_type.ContainsGenericParameter)
				{
					return base.ContainsGenericParameter;
				}
				return true;
			}
		}

		internal FieldReference()
		{
			token = new MetadataToken(TokenType.MemberRef);
		}

		public FieldReference(string name, TypeReference fieldType)
			: base(name)
		{
			if (fieldType == null)
			{
				throw new ArgumentNullException("fieldType");
			}
			field_type = fieldType;
			token = new MetadataToken(TokenType.MemberRef);
		}

		public FieldReference(string name, TypeReference fieldType, TypeReference declaringType)
			: this(name, fieldType)
		{
			if (declaringType == null)
			{
				throw new ArgumentNullException("declaringType");
			}
			DeclaringType = declaringType;
		}

		public virtual FieldDefinition Resolve()
		{
			ModuleDefinition module = Module;
			if (module == null)
			{
				throw new NotSupportedException();
			}
			return module.Resolve(this);
		}
	}
}
