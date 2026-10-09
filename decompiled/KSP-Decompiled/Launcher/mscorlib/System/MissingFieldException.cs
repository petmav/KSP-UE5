using System.Runtime.InteropServices;
using System.Runtime.Serialization;
using System.Security;

namespace System;

[Serializable]
[ComVisible(true)]
public class MissingFieldException : MissingMemberException, ISerializable
{
	public override string Message
	{
		[SecuritySafeCritical]
		get
		{
			if (ClassName == null)
			{
				return base.Message;
			}
			return Environment.GetResourceString("Field '{0}' not found.", ((Signature != null) ? (MissingMemberException.FormatSignature(Signature) + " ") : "") + ClassName + "." + MemberName);
		}
	}

	public MissingFieldException()
		: base(Environment.GetResourceString("Attempted to access a non-existing field."))
	{
		SetErrorCode(-2146233071);
	}

	public MissingFieldException(string message)
		: base(message)
	{
		SetErrorCode(-2146233071);
	}

	public MissingFieldException(string message, Exception inner)
		: base(message, inner)
	{
		SetErrorCode(-2146233071);
	}

	protected MissingFieldException(SerializationInfo info, StreamingContext context)
		: base(info, context)
	{
	}

	private MissingFieldException(string className, string fieldName, byte[] signature)
	{
		ClassName = className;
		MemberName = fieldName;
		Signature = signature;
	}

	public MissingFieldException(string className, string fieldName)
	{
		ClassName = className;
		MemberName = fieldName;
	}
}
