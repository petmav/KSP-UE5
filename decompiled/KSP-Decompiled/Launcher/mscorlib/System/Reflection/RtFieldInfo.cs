using System.Diagnostics;
using System.Globalization;
using System.Runtime.CompilerServices;

namespace System.Reflection;

internal abstract class RtFieldInfo : RuntimeFieldInfo
{
	[MethodImpl(MethodImplOptions.InternalCall)]
	internal extern object UnsafeGetValue(object obj);

	internal void CheckConsistency(object target)
	{
		if ((Attributes & FieldAttributes.Static) != FieldAttributes.Static && !DeclaringType.IsInstanceOfType(target))
		{
			if (target == null)
			{
				throw new TargetException(Environment.GetResourceString("Non-static field requires a target."));
			}
			throw new ArgumentException(string.Format(CultureInfo.CurrentUICulture, Environment.GetResourceString("Field '{0}' defined on type '{1}' is not a field on the target object which is of type '{2}'."), Name, DeclaringType, target.GetType()));
		}
	}

	[DebuggerStepThrough]
	[DebuggerHidden]
	internal void UnsafeSetValue(object obj, object value, BindingFlags invokeAttr, Binder binder, CultureInfo culture)
	{
		bool domainInitialized = false;
		RuntimeFieldHandle.SetValue(this, obj, value, null, Attributes, null, ref domainInitialized);
	}

	[DebuggerHidden]
	[DebuggerStepThrough]
	public unsafe override void SetValueDirect(TypedReference obj, object value)
	{
		if (obj.IsNull)
		{
			throw new ArgumentException(Environment.GetResourceString("The TypedReference must be initialized."));
		}
		RuntimeFieldHandle.SetValueDirect(this, (RuntimeType)FieldType, &obj, value, (RuntimeType)DeclaringType);
	}

	[DebuggerHidden]
	[DebuggerStepThrough]
	public unsafe override object GetValueDirect(TypedReference obj)
	{
		if (obj.IsNull)
		{
			throw new ArgumentException(Environment.GetResourceString("The TypedReference must be initialized."));
		}
		return RuntimeFieldHandle.GetValueDirect(this, (RuntimeType)FieldType, &obj, (RuntimeType)DeclaringType);
	}
}
