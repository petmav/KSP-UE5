using System.Runtime.InteropServices;
using System.Runtime.Serialization;
using System.Security;

namespace System;

[Serializable]
[ComVisible(true)]
public class ArgumentOutOfRangeException : ArgumentException, ISerializable
{
	private static volatile string _rangeMessage;

	private object m_actualValue;

	private static string RangeMessage
	{
		get
		{
			if (_rangeMessage == null)
			{
				_rangeMessage = Environment.GetResourceString("Specified argument was out of the range of valid values.");
			}
			return _rangeMessage;
		}
	}

	public override string Message
	{
		get
		{
			string message = base.Message;
			if (m_actualValue != null)
			{
				string resourceString = Environment.GetResourceString("Actual value was {0}.", m_actualValue.ToString());
				if (message == null)
				{
					return resourceString;
				}
				return message + Environment.NewLine + resourceString;
			}
			return message;
		}
	}

	public virtual object ActualValue => m_actualValue;

	public ArgumentOutOfRangeException()
		: base(RangeMessage)
	{
		SetErrorCode(-2146233086);
	}

	public ArgumentOutOfRangeException(string paramName)
		: base(RangeMessage, paramName)
	{
		SetErrorCode(-2146233086);
	}

	public ArgumentOutOfRangeException(string paramName, string message)
		: base(message, paramName)
	{
		SetErrorCode(-2146233086);
	}

	public ArgumentOutOfRangeException(string message, Exception innerException)
		: base(message, innerException)
	{
		SetErrorCode(-2146233086);
	}

	public ArgumentOutOfRangeException(string paramName, object actualValue, string message)
		: base(message, paramName)
	{
		m_actualValue = actualValue;
		SetErrorCode(-2146233086);
	}

	[SecurityCritical]
	public override void GetObjectData(SerializationInfo info, StreamingContext context)
	{
		if (info == null)
		{
			throw new ArgumentNullException("info");
		}
		base.GetObjectData(info, context);
		info.AddValue("ActualValue", m_actualValue, typeof(object));
	}

	protected ArgumentOutOfRangeException(SerializationInfo info, StreamingContext context)
		: base(info, context)
	{
		m_actualValue = info.GetValue("ActualValue", typeof(object));
	}
}
