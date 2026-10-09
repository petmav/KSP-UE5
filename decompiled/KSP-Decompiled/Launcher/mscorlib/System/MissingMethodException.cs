using System.Globalization;
using System.Runtime.InteropServices;
using System.Runtime.Serialization;
using System.Security;

namespace System;

[Serializable]
[ComVisible(true)]
public class MissingMethodException : MissingMemberException, ISerializable
{
	[NonSerialized]
	private string signature;

	public override string Message
	{
		[SecuritySafeCritical]
		get
		{
			if (ClassName == null)
			{
				return base.Message;
			}
			string text = ClassName + "." + MemberName;
			if (!string.IsNullOrEmpty(signature))
			{
				text = string.Format(CultureInfo.InvariantCulture, signature, text);
			}
			if (!string.IsNullOrEmpty(_message))
			{
				text = text + " Due to: " + _message;
			}
			return text;
		}
	}

	public MissingMethodException()
		: base(Environment.GetResourceString("Attempted to access a missing method."))
	{
		SetErrorCode(-2146233069);
	}

	public MissingMethodException(string message)
		: base(message)
	{
		SetErrorCode(-2146233069);
	}

	public MissingMethodException(string message, Exception inner)
		: base(message, inner)
	{
		SetErrorCode(-2146233069);
	}

	protected MissingMethodException(SerializationInfo info, StreamingContext context)
		: base(info, context)
	{
	}

	private MissingMethodException(string className, string methodName, byte[] signature)
	{
		ClassName = className;
		MemberName = methodName;
		Signature = signature;
	}

	public MissingMethodException(string className, string methodName)
	{
		ClassName = className;
		MemberName = methodName;
	}

	private MissingMethodException(string className, string methodName, string signature, string message)
		: base(message)
	{
		ClassName = className;
		MemberName = methodName;
		this.signature = signature;
	}
}
