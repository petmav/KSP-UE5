using System.Collections;
using System.ComponentModel.Design;
using System.Reflection;
using System.Security.Permissions;

namespace System.ComponentModel;

[HostProtection(SecurityAction.LinkDemand, SharedState = true)]
internal sealed class ReflectEventDescriptor : EventDescriptor
{
	private Type type;

	private readonly Type componentClass;

	private MethodInfo addMethod;

	private MethodInfo removeMethod;

	private EventInfo realEvent;

	private bool filledMethods;

	public override Type ComponentType => componentClass;

	public override Type EventType
	{
		get
		{
			FillMethods();
			return type;
		}
	}

	public override bool IsMulticast => typeof(MulticastDelegate).IsAssignableFrom(EventType);

	public ReflectEventDescriptor(Type componentClass, string name, Type type, Attribute[] attributes)
		: base(name, attributes)
	{
		if (componentClass == null)
		{
			throw new ArgumentException(global::SR.GetString("Null is not a valid value for {0}.", "componentClass"));
		}
		if (type == null || !typeof(Delegate).IsAssignableFrom(type))
		{
			throw new ArgumentException(global::SR.GetString("Invalid type for the {0} event.", name));
		}
		this.componentClass = componentClass;
		this.type = type;
	}

	public ReflectEventDescriptor(Type componentClass, EventInfo eventInfo)
		: base(eventInfo.Name, new Attribute[0])
	{
		if (componentClass == null)
		{
			throw new ArgumentException(global::SR.GetString("Null is not a valid value for {0}.", "componentClass"));
		}
		this.componentClass = componentClass;
		realEvent = eventInfo;
	}

	public ReflectEventDescriptor(Type componentType, EventDescriptor oldReflectEventDescriptor, Attribute[] attributes)
		: base(oldReflectEventDescriptor, attributes)
	{
		componentClass = componentType;
		type = oldReflectEventDescriptor.EventType;
		if (oldReflectEventDescriptor is ReflectEventDescriptor reflectEventDescriptor)
		{
			addMethod = reflectEventDescriptor.addMethod;
			removeMethod = reflectEventDescriptor.removeMethod;
			filledMethods = true;
		}
	}

	public override void AddEventHandler(object component, Delegate value)
	{
		FillMethods();
		if (component == null)
		{
			return;
		}
		ISite site = MemberDescriptor.GetSite(component);
		IComponentChangeService componentChangeService = null;
		if (site != null)
		{
			componentChangeService = (IComponentChangeService)site.GetService(typeof(IComponentChangeService));
		}
		if (componentChangeService != null)
		{
			try
			{
				componentChangeService.OnComponentChanging(component, this);
			}
			catch (CheckoutException ex)
			{
				if (ex == CheckoutException.Canceled)
				{
					return;
				}
				throw ex;
			}
		}
		bool flag = false;
		if (site != null && site.DesignMode)
		{
			if (EventType != value.GetType())
			{
				throw new ArgumentException(global::SR.GetString("Invalid event handler for the {0} event.", Name));
			}
			IDictionaryService dictionaryService = (IDictionaryService)site.GetService(typeof(IDictionaryService));
			if (dictionaryService != null)
			{
				Delegate a = (Delegate)dictionaryService.GetValue(this);
				a = Delegate.Combine(a, value);
				dictionaryService.SetValue(this, a);
				flag = true;
			}
		}
		if (!flag)
		{
			SecurityUtils.MethodInfoInvoke(addMethod, component, new object[1] { value });
		}
		componentChangeService?.OnComponentChanged(component, this, null, value);
	}

	protected override void FillAttributes(IList attributes)
	{
		FillMethods();
		if (realEvent != null)
		{
			FillEventInfoAttribute(realEvent, attributes);
		}
		else
		{
			FillSingleMethodAttribute(removeMethod, attributes);
			FillSingleMethodAttribute(addMethod, attributes);
		}
		base.FillAttributes(attributes);
	}

	private void FillEventInfoAttribute(EventInfo realEventInfo, IList attributes)
	{
		string text = realEventInfo.Name;
		BindingFlags bindingAttr = BindingFlags.DeclaredOnly | BindingFlags.Instance | BindingFlags.Public;
		Type type = realEventInfo.ReflectedType;
		int num = 0;
		while (type != typeof(object))
		{
			num++;
			type = type.BaseType;
		}
		if (num <= 0)
		{
			return;
		}
		type = realEventInfo.ReflectedType;
		Attribute[][] array = new Attribute[num][];
		while (type != typeof(object))
		{
			MemberInfo @event = type.GetEvent(text, bindingAttr);
			if (@event != null)
			{
				array[--num] = ReflectTypeDescriptionProvider.ReflectGetAttributes(@event);
			}
			type = type.BaseType;
		}
		Attribute[][] array2 = array;
		foreach (Attribute[] array3 in array2)
		{
			if (array3 != null)
			{
				Attribute[] array4 = array3;
				foreach (Attribute value in array4)
				{
					attributes.Add(value);
				}
			}
		}
	}

	private void FillMethods()
	{
		if (filledMethods)
		{
			return;
		}
		if (realEvent != null)
		{
			addMethod = realEvent.GetAddMethod();
			removeMethod = realEvent.GetRemoveMethod();
			EventInfo eventInfo = null;
			if (addMethod == null || removeMethod == null)
			{
				Type baseType = componentClass.BaseType;
				while (baseType != null && baseType != typeof(object))
				{
					BindingFlags bindingAttr = BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic;
					EventInfo @event = baseType.GetEvent(realEvent.Name, bindingAttr);
					if (@event.GetAddMethod() != null)
					{
						eventInfo = @event;
						break;
					}
				}
			}
			if (eventInfo != null)
			{
				addMethod = eventInfo.GetAddMethod();
				removeMethod = eventInfo.GetRemoveMethod();
				type = eventInfo.EventHandlerType;
			}
			else
			{
				type = realEvent.EventHandlerType;
			}
		}
		else
		{
			realEvent = componentClass.GetEvent(Name);
			if (realEvent != null)
			{
				FillMethods();
				return;
			}
			Type[] args = new Type[1] { type };
			addMethod = MemberDescriptor.FindMethod(componentClass, "AddOn" + Name, args, typeof(void));
			removeMethod = MemberDescriptor.FindMethod(componentClass, "RemoveOn" + Name, args, typeof(void));
			if (addMethod == null || removeMethod == null)
			{
				throw new ArgumentException(global::SR.GetString("Accessor methods for the {0} event are missing.", Name));
			}
		}
		filledMethods = true;
	}

	private void FillSingleMethodAttribute(MethodInfo realMethodInfo, IList attributes)
	{
		string text = realMethodInfo.Name;
		BindingFlags bindingAttr = BindingFlags.DeclaredOnly | BindingFlags.Instance | BindingFlags.Public;
		Type type = realMethodInfo.ReflectedType;
		int num = 0;
		while (type != null && type != typeof(object))
		{
			num++;
			type = type.BaseType;
		}
		if (num <= 0)
		{
			return;
		}
		type = realMethodInfo.ReflectedType;
		Attribute[][] array = new Attribute[num][];
		while (type != null && type != typeof(object))
		{
			MemberInfo method = type.GetMethod(text, bindingAttr);
			if (method != null)
			{
				array[--num] = ReflectTypeDescriptionProvider.ReflectGetAttributes(method);
			}
			type = type.BaseType;
		}
		Attribute[][] array2 = array;
		foreach (Attribute[] array3 in array2)
		{
			if (array3 != null)
			{
				Attribute[] array4 = array3;
				foreach (Attribute value in array4)
				{
					attributes.Add(value);
				}
			}
		}
	}

	public override void RemoveEventHandler(object component, Delegate value)
	{
		FillMethods();
		if (component == null)
		{
			return;
		}
		ISite site = MemberDescriptor.GetSite(component);
		IComponentChangeService componentChangeService = null;
		if (site != null)
		{
			componentChangeService = (IComponentChangeService)site.GetService(typeof(IComponentChangeService));
		}
		if (componentChangeService != null)
		{
			try
			{
				componentChangeService.OnComponentChanging(component, this);
			}
			catch (CheckoutException ex)
			{
				if (ex == CheckoutException.Canceled)
				{
					return;
				}
				throw ex;
			}
		}
		bool flag = false;
		if (site != null && site.DesignMode)
		{
			IDictionaryService dictionaryService = (IDictionaryService)site.GetService(typeof(IDictionaryService));
			if (dictionaryService != null)
			{
				Delegate source = (Delegate)dictionaryService.GetValue(this);
				source = Delegate.Remove(source, value);
				dictionaryService.SetValue(this, source);
				flag = true;
			}
		}
		if (!flag)
		{
			SecurityUtils.MethodInfoInvoke(removeMethod, component, new object[1] { value });
		}
		componentChangeService?.OnComponentChanged(component, this, null, value);
	}
}
