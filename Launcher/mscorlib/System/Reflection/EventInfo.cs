using System.Diagnostics;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using Mono;
using Unity;

namespace System.Reflection;

[Serializable]
[StructLayout(LayoutKind.Sequential)]
[ComDefaultInterface(typeof(_EventInfo))]
[ClassInterface(ClassInterfaceType.None)]
[ComVisible(true)]
public abstract class EventInfo : MemberInfo, _EventInfo
{
	private delegate void AddEventAdapter(object _this, Delegate dele);

	private AddEventAdapter cached_add_event;

	public abstract EventAttributes Attributes { get; }

	public virtual Type EventHandlerType
	{
		get
		{
			ParameterInfo[] parametersInternal = GetAddMethod(nonPublic: true).GetParametersInternal();
			if (parametersInternal.Length != 0)
			{
				return parametersInternal[0].ParameterType;
			}
			return null;
		}
	}

	public virtual bool IsMulticast => true;

	public bool IsSpecialName => (Attributes & EventAttributes.SpecialName) != 0;

	public override MemberTypes MemberType => MemberTypes.Event;

	public virtual MethodInfo AddMethod => GetAddMethod(nonPublic: true);

	public virtual MethodInfo RaiseMethod => GetRaiseMethod(nonPublic: true);

	public virtual MethodInfo RemoveMethod => GetRemoveMethod(nonPublic: true);

	[DebuggerStepThrough]
	[DebuggerHidden]
	public virtual void AddEventHandler(object target, Delegate handler)
	{
		MethodInfo addMethod = GetAddMethod();
		if (addMethod == null)
		{
			throw new InvalidOperationException("Cannot add a handler to an event that doesn't have a visible add method");
		}
		if (target == null && !addMethod.IsStatic)
		{
			throw new TargetException("Cannot add a handler to a non static event with a null target");
		}
		addMethod.Invoke(target, new object[1] { handler });
	}

	public MethodInfo GetAddMethod()
	{
		return GetAddMethod(nonPublic: false);
	}

	public abstract MethodInfo GetAddMethod(bool nonPublic);

	public MethodInfo GetRaiseMethod()
	{
		return GetRaiseMethod(nonPublic: false);
	}

	public abstract MethodInfo GetRaiseMethod(bool nonPublic);

	public MethodInfo GetRemoveMethod()
	{
		return GetRemoveMethod(nonPublic: false);
	}

	public abstract MethodInfo GetRemoveMethod(bool nonPublic);

	public virtual MethodInfo[] GetOtherMethods(bool nonPublic)
	{
		return EmptyArray<MethodInfo>.Value;
	}

	public MethodInfo[] GetOtherMethods()
	{
		return GetOtherMethods(nonPublic: false);
	}

	[DebuggerHidden]
	[DebuggerStepThrough]
	public virtual void RemoveEventHandler(object target, Delegate handler)
	{
		MethodInfo removeMethod = GetRemoveMethod();
		if (removeMethod == null)
		{
			throw new InvalidOperationException("Cannot remove a handler to an event that doesn't have a visible remove method");
		}
		removeMethod.Invoke(target, new object[1] { handler });
	}

	public override bool Equals(object obj)
	{
		return obj == this;
	}

	public override int GetHashCode()
	{
		return base.GetHashCode();
	}

	public static bool operator ==(EventInfo left, EventInfo right)
	{
		if ((object)left == right)
		{
			return true;
		}
		if (((object)left == null) ^ ((object)right == null))
		{
			return false;
		}
		return left.Equals(right);
	}

	public static bool operator !=(EventInfo left, EventInfo right)
	{
		if ((object)left == right)
		{
			return false;
		}
		if (((object)left == null) ^ ((object)right == null))
		{
			return true;
		}
		return !left.Equals(right);
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern EventInfo internal_from_handle_type(IntPtr event_handle, IntPtr type_handle);

	internal static EventInfo GetEventFromHandle(RuntimeEventHandle handle)
	{
		if (handle.Value == IntPtr.Zero)
		{
			throw new ArgumentException("The handle is invalid.");
		}
		return internal_from_handle_type(handle.Value, IntPtr.Zero);
	}

	internal static EventInfo GetEventFromHandle(RuntimeEventHandle handle, RuntimeTypeHandle reflectedType)
	{
		if (handle.Value == IntPtr.Zero)
		{
			throw new ArgumentException("The handle is invalid.");
		}
		EventInfo eventInfo = internal_from_handle_type(handle.Value, reflectedType.Value);
		if (eventInfo == null)
		{
			throw new ArgumentException("The event handle and the type handle are incompatible.");
		}
		return eventInfo;
	}

	void _EventInfo.GetIDsOfNames([In] ref Guid riid, IntPtr rgszNames, uint cNames, uint lcid, IntPtr rgDispId)
	{
		ThrowStub.ThrowNotSupportedException();
	}

	Type _EventInfo.GetType()
	{
		ThrowStub.ThrowNotSupportedException();
		return null;
	}

	void _EventInfo.GetTypeInfo(uint iTInfo, uint lcid, IntPtr ppTInfo)
	{
		ThrowStub.ThrowNotSupportedException();
	}

	void _EventInfo.GetTypeInfoCount(out uint pcTInfo)
	{
		ThrowStub.ThrowNotSupportedException();
	}

	void _EventInfo.Invoke(uint dispIdMember, [In] ref Guid riid, uint lcid, short wFlags, IntPtr pDispParams, IntPtr pVarResult, IntPtr pExcepInfo, IntPtr puArgErr)
	{
		ThrowStub.ThrowNotSupportedException();
	}
}
