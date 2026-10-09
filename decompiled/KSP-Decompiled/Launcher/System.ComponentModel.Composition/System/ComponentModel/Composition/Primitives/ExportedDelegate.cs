using System.Reflection;
using Microsoft.Internal;

namespace System.ComponentModel.Composition.Primitives;

public class ExportedDelegate
{
	private object _instance;

	private MethodInfo _method;

	protected ExportedDelegate()
	{
	}

	public ExportedDelegate(object instance, MethodInfo method)
	{
		Requires.NotNull(method, "method");
		_instance = instance;
		_method = method;
	}

	public virtual Delegate CreateDelegate(Type delegateType)
	{
		Requires.NotNull(delegateType, "delegateType");
		if (delegateType == typeof(Delegate) || delegateType == typeof(MulticastDelegate))
		{
			delegateType = CreateStandardDelegateType();
		}
		return Delegate.CreateDelegate(delegateType, _instance, _method, throwOnBindFailure: false);
	}

	private Type CreateStandardDelegateType()
	{
		throw new NotImplementedException();
	}
}
