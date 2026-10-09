using System.Diagnostics;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using Mono;

namespace System.Reflection;

[Serializable]
[StructLayout(LayoutKind.Sequential)]
[ComDefaultInterface(typeof(_EventInfo))]
[ClassInterface(ClassInterfaceType.None)]
[ComVisible(true)]
public abstract class EventInfo : MemberInfo, _EventInfo
{
	private delegate void AddEventAdapter(object _this, Delegate dele);

	private delegate void AddEvent<T, D>(T _this, D dele);

	private delegate void StaticAddEvent<D>(D dele);

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

	[DebuggerHidden]
	[DebuggerStepThrough]
	public virtual void AddEventHandler(object target, Delegate handler)
	{
		if (cached_add_event == null)
		{
			MethodInfo addMethod = GetAddMethod();
			if (addMethod == null)
			{
				throw new InvalidOperationException("Cannot add a handler to an event that doesn't have a visible add method");
			}
			if (addMethod.DeclaringType.IsValueType)
			{
				if (target == null && !addMethod.IsStatic)
				{
					throw new TargetException("Cannot add a handler to a non static event with a null target");
				}
				addMethod.Invoke(target, new object[1] { handler });
				return;
			}
			cached_add_event = CreateAddEventDelegate(addMethod);
		}
		cached_add_event(target, handler);
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

	[DebuggerStepThrough]
	[DebuggerHidden]
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

	void _EventInfo.GetIDsOfNames([In] ref Guid riid, IntPtr rgszNames, uint cNames, uint lcid, IntPtr rgDispId)
	{
		throw new NotImplementedException();
	}

	Type _EventInfo.GetType()
	{
		return GetType();
	}

	void _EventInfo.GetTypeInfo(uint iTInfo, uint lcid, IntPtr ppTInfo)
	{
		throw new NotImplementedException();
	}

	void _EventInfo.GetTypeInfoCount(out uint pcTInfo)
	{
		throw new NotImplementedException();
	}

	void _EventInfo.Invoke(uint dispIdMember, [In] ref Guid riid, uint lcid, short wFlags, IntPtr pDispParams, IntPtr pVarResult, IntPtr pExcepInfo, IntPtr puArgErr)
	{
		throw new NotImplementedException();
	}

	private static void AddEventFrame<T, D>(AddEvent<T, D> addEvent, object obj, object dele)
	{
		if (obj == null)
		{
			throw new TargetException("Cannot add a handler to a non static event with a null target");
		}
		if (!(obj is T))
		{
			throw new TargetException("Object doesn't match target");
		}
		addEvent((T)obj, (D)dele);
	}

	private static void StaticAddEventAdapterFrame<D>(StaticAddEvent<D> addEvent, object obj, object dele)
	{
		addEvent((D)dele);
	}

	private static AddEventAdapter CreateAddEventDelegate(MethodInfo method)
	{
		Type[] typeArguments;
		Type typeFromHandle;
		string name;
		if (method.IsStatic)
		{
			typeArguments = new Type[1] { method.GetParametersInternal()[0].ParameterType };
			typeFromHandle = typeof(StaticAddEvent<>);
			name = "StaticAddEventAdapterFrame";
		}
		else
		{
			typeArguments = new Type[2]
			{
				method.DeclaringType,
				method.GetParametersInternal()[0].ParameterType
			};
			typeFromHandle = typeof(AddEvent<, >);
			name = "AddEventFrame";
		}
		object firstArgument = Delegate.CreateDelegate(typeFromHandle.MakeGenericType(typeArguments), method);
		MethodInfo method2 = typeof(EventInfo).GetMethod(name, BindingFlags.Static | BindingFlags.NonPublic);
		method2 = method2.MakeGenericMethod(typeArguments);
		return (AddEventAdapter)Delegate.CreateDelegate(typeof(AddEventAdapter), firstArgument, method2, throwOnBindFailure: true);
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
}
