using System.Collections.Generic;
using System.Globalization;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace System.Reflection;

[Serializable]
[StructLayout(LayoutKind.Sequential)]
internal class MonoField : RtFieldInfo
{
	internal IntPtr klass;

	internal RuntimeFieldHandle fhandle;

	private string name;

	private Type type;

	private FieldAttributes attrs;

	public override FieldAttributes Attributes => attrs;

	public override RuntimeFieldHandle FieldHandle => fhandle;

	public override Type FieldType
	{
		get
		{
			if (type == null)
			{
				type = ResolveType();
			}
			return type;
		}
	}

	public override Type ReflectedType => GetParentType(declaring: false);

	public override Type DeclaringType => GetParentType(declaring: true);

	public override string Name => name;

	public override bool IsSecurityTransparent => get_core_clr_security_level() == 0;

	public override bool IsSecurityCritical => get_core_clr_security_level() > 0;

	public override bool IsSecuritySafeCritical => get_core_clr_security_level() == 1;

	[MethodImpl(MethodImplOptions.InternalCall)]
	private extern Type ResolveType();

	[MethodImpl(MethodImplOptions.InternalCall)]
	private extern Type GetParentType(bool declaring);

	public override bool IsDefined(Type attributeType, bool inherit)
	{
		return MonoCustomAttrs.IsDefined(this, attributeType, inherit);
	}

	public override object[] GetCustomAttributes(bool inherit)
	{
		return MonoCustomAttrs.GetCustomAttributes(this, inherit);
	}

	public override object[] GetCustomAttributes(Type attributeType, bool inherit)
	{
		return MonoCustomAttrs.GetCustomAttributes(this, attributeType, inherit);
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	internal override extern int GetFieldOffset();

	[MethodImpl(MethodImplOptions.InternalCall)]
	private extern object GetValueInternal(object obj);

	public override object GetValue(object obj)
	{
		if (!base.IsStatic)
		{
			if (obj == null)
			{
				throw new TargetException("Non-static field requires a target");
			}
			if (!DeclaringType.IsAssignableFrom(obj.GetType()))
			{
				throw new ArgumentException($"Field {Name} defined on type {DeclaringType} is not a field on the target object which is of type {obj.GetType()}.", "obj");
			}
		}
		if (!base.IsLiteral)
		{
			CheckGeneric();
		}
		return GetValueInternal(obj);
	}

	public override string ToString()
	{
		return $"{FieldType} {name}";
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern void SetValueInternal(FieldInfo fi, object obj, object value);

	public override void SetValue(object obj, object val, BindingFlags invokeAttr, Binder binder, CultureInfo culture)
	{
		if (!base.IsStatic)
		{
			if (obj == null)
			{
				throw new TargetException("Non-static field requires a target");
			}
			if (!DeclaringType.IsAssignableFrom(obj.GetType()))
			{
				throw new ArgumentException($"Field {Name} defined on type {DeclaringType} is not a field on the target object which is of type {obj.GetType()}.", "obj");
			}
		}
		if (base.IsLiteral)
		{
			throw new FieldAccessException("Cannot set a constant field");
		}
		if (binder == null)
		{
			binder = Type.DefaultBinder;
		}
		CheckGeneric();
		if (val != null)
		{
			val = ((RuntimeType)FieldType).CheckValue(val, binder, culture, invokeAttr);
		}
		SetValueInternal(this, obj, val);
	}

	internal MonoField Clone(string newName)
	{
		return new MonoField
		{
			name = newName,
			type = type,
			attrs = attrs,
			klass = klass,
			fhandle = fhandle
		};
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	public override extern object GetRawConstantValue();

	public override IList<CustomAttributeData> GetCustomAttributesData()
	{
		return CustomAttributeData.GetCustomAttributes(this);
	}

	private void CheckGeneric()
	{
		if (DeclaringType.ContainsGenericParameters)
		{
			throw new InvalidOperationException("Late bound operations cannot be performed on fields with types for which Type.ContainsGenericParameters is true.");
		}
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	public extern int get_core_clr_security_level();
}
