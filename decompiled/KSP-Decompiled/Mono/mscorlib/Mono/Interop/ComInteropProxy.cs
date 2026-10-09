using System;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Runtime.Remoting;
using System.Runtime.Remoting.Messaging;
using System.Runtime.Remoting.Proxies;
using System.Threading;

namespace Mono.Interop;

[StructLayout(LayoutKind.Sequential)]
internal class ComInteropProxy : RealProxy, IRemotingTypeInfo
{
	private __ComObject com_object;

	private int ref_count = 1;

	private string type_name;

	public string TypeName
	{
		get
		{
			return type_name;
		}
		set
		{
			type_name = value;
		}
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	private static extern void AddProxy(IntPtr pItf, ComInteropProxy proxy);

	[MethodImpl(MethodImplOptions.InternalCall)]
	internal static extern ComInteropProxy FindProxy(IntPtr pItf);

	private ComInteropProxy(Type t)
		: base(t)
	{
		com_object = __ComObject.CreateRCW(t);
	}

	private void CacheProxy()
	{
		if (FindProxy(com_object.IUnknown) == null)
		{
			AddProxy(com_object.IUnknown, this);
		}
		else
		{
			Interlocked.Increment(ref ref_count);
		}
	}

	private ComInteropProxy(IntPtr pUnk)
		: this(pUnk, typeof(__ComObject))
	{
	}

	internal ComInteropProxy(IntPtr pUnk, Type t)
		: base(t)
	{
		com_object = new __ComObject(pUnk, this);
		CacheProxy();
	}

	internal static ComInteropProxy GetProxy(IntPtr pItf, Type t)
	{
		Guid iid = __ComObject.IID_IUnknown;
		Marshal.ThrowExceptionForHR(Marshal.QueryInterface(pItf, ref iid, out var ppv));
		ComInteropProxy comInteropProxy = FindProxy(ppv);
		if (comInteropProxy == null)
		{
			Marshal.Release(ppv);
			return new ComInteropProxy(ppv);
		}
		Marshal.Release(ppv);
		Interlocked.Increment(ref comInteropProxy.ref_count);
		return comInteropProxy;
	}

	internal static ComInteropProxy CreateProxy(Type t)
	{
		IntPtr intPtr = __ComObject.CreateIUnknown(t);
		ComInteropProxy comInteropProxy = FindProxy(intPtr);
		ComInteropProxy comInteropProxy2;
		if (comInteropProxy != null)
		{
			Type type = comInteropProxy.com_object.GetType();
			if (type != t)
			{
				throw new InvalidCastException($"Unable to cast object of type '{type}' to type '{t}'.");
			}
			comInteropProxy2 = comInteropProxy;
			Marshal.Release(intPtr);
		}
		else
		{
			comInteropProxy2 = new ComInteropProxy(t);
			comInteropProxy2.com_object.Initialize(intPtr, comInteropProxy2);
		}
		return comInteropProxy2;
	}

	public override IMessage Invoke(IMessage msg)
	{
		Console.WriteLine("Invoke");
		Console.WriteLine(Environment.StackTrace);
		throw new Exception("The method or operation is not implemented.");
	}

	public bool CanCastTo(Type fromType, object o)
	{
		if (!(o is __ComObject _ComObject))
		{
			throw new NotSupportedException("Only RCWs are currently supported");
		}
		if ((fromType.Attributes & TypeAttributes.Import) == 0)
		{
			return false;
		}
		if (_ComObject.GetInterface(fromType, throwException: false) == IntPtr.Zero)
		{
			return false;
		}
		return true;
	}
}
