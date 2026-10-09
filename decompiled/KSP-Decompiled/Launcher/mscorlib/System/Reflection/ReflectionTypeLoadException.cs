using System.Runtime.InteropServices;
using System.Runtime.Serialization;
using System.Security;

namespace System.Reflection;

[Serializable]
[ComVisible(true)]
public sealed class ReflectionTypeLoadException : SystemException, ISerializable
{
	private Type[] _classes;

	private Exception[] _exceptions;

	public Type[] Types => _classes;

	public Exception[] LoaderExceptions => _exceptions;

	private ReflectionTypeLoadException()
		: base(Environment.GetResourceString("Unable to load one or more of the requested types. Retrieve the LoaderExceptions property for more information."))
	{
		SetErrorCode(-2146232830);
	}

	private ReflectionTypeLoadException(string message)
		: base(message)
	{
		SetErrorCode(-2146232830);
	}

	public ReflectionTypeLoadException(Type[] classes, Exception[] exceptions)
		: base(null)
	{
		_classes = classes;
		_exceptions = exceptions;
		SetErrorCode(-2146232830);
	}

	public ReflectionTypeLoadException(Type[] classes, Exception[] exceptions, string message)
		: base(message)
	{
		_classes = classes;
		_exceptions = exceptions;
		SetErrorCode(-2146232830);
	}

	internal ReflectionTypeLoadException(SerializationInfo info, StreamingContext context)
		: base(info, context)
	{
		_classes = (Type[])info.GetValue("Types", typeof(Type[]));
		_exceptions = (Exception[])info.GetValue("Exceptions", typeof(Exception[]));
	}

	[SecurityCritical]
	public override void GetObjectData(SerializationInfo info, StreamingContext context)
	{
		if (info == null)
		{
			throw new ArgumentNullException("info");
		}
		base.GetObjectData(info, context);
		info.AddValue("Types", _classes, typeof(Type[]));
		info.AddValue("Exceptions", _exceptions, typeof(Exception[]));
	}
}
