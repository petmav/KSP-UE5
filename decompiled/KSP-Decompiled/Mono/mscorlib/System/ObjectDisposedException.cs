using System.Runtime.InteropServices;
using System.Runtime.Serialization;
using System.Security;

namespace System;

[Serializable]
[ComVisible(true)]
public class ObjectDisposedException : InvalidOperationException
{
	private string objectName;

	public override string Message
	{
		get
		{
			string text = ObjectName;
			if (text == null || text.Length == 0)
			{
				return base.Message;
			}
			string resourceString = Environment.GetResourceString("Object name: '{0}'.", text);
			return base.Message + Environment.NewLine + resourceString;
		}
	}

	public string ObjectName
	{
		get
		{
			if (objectName == null && !CompatibilitySwitches.IsAppEarlierThanWindowsPhone8)
			{
				return string.Empty;
			}
			return objectName;
		}
	}

	private ObjectDisposedException()
		: this(null, Environment.GetResourceString("Cannot access a disposed object."))
	{
	}

	public ObjectDisposedException(string objectName)
		: this(objectName, Environment.GetResourceString("Cannot access a disposed object."))
	{
	}

	public ObjectDisposedException(string objectName, string message)
		: base(message)
	{
		SetErrorCode(-2146232798);
		this.objectName = objectName;
	}

	public ObjectDisposedException(string message, Exception innerException)
		: base(message, innerException)
	{
		SetErrorCode(-2146232798);
	}

	protected ObjectDisposedException(SerializationInfo info, StreamingContext context)
		: base(info, context)
	{
		objectName = info.GetString("ObjectName");
	}

	[SecurityCritical]
	public override void GetObjectData(SerializationInfo info, StreamingContext context)
	{
		base.GetObjectData(info, context);
		info.AddValue("ObjectName", ObjectName, typeof(string));
	}
}
