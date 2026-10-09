using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Runtime.Serialization;
using System.Security;

namespace System;

[Serializable]
[ComVisible(true)]
public class MissingMemberException : MemberAccessException, ISerializable
{
	protected string ClassName;

	protected string MemberName;

	protected byte[] Signature;

	public override string Message
	{
		[SecuritySafeCritical]
		get
		{
			if (ClassName == null)
			{
				return base.Message;
			}
			return Environment.GetResourceString("Member '{0}' not found.", ClassName + "." + MemberName + ((Signature != null) ? (" " + FormatSignature(Signature)) : ""));
		}
	}

	public MissingMemberException()
		: base(Environment.GetResourceString("Attempted to access a missing member."))
	{
		SetErrorCode(-2146233070);
	}

	public MissingMemberException(string message)
		: base(message)
	{
		SetErrorCode(-2146233070);
	}

	public MissingMemberException(string message, Exception inner)
		: base(message, inner)
	{
		SetErrorCode(-2146233070);
	}

	protected MissingMemberException(SerializationInfo info, StreamingContext context)
		: base(info, context)
	{
		ClassName = info.GetString("MMClassName");
		MemberName = info.GetString("MMMemberName");
		Signature = (byte[])info.GetValue("MMSignature", typeof(byte[]));
	}

	[MethodImpl(MethodImplOptions.InternalCall)]
	[SecurityCritical]
	internal static extern string FormatSignature(byte[] signature);

	private MissingMemberException(string className, string memberName, byte[] signature)
	{
		ClassName = className;
		MemberName = memberName;
		Signature = signature;
	}

	public MissingMemberException(string className, string memberName)
	{
		ClassName = className;
		MemberName = memberName;
	}

	[SecurityCritical]
	public override void GetObjectData(SerializationInfo info, StreamingContext context)
	{
		if (info == null)
		{
			throw new ArgumentNullException("info");
		}
		base.GetObjectData(info, context);
		info.AddValue("MMClassName", ClassName, typeof(string));
		info.AddValue("MMMemberName", MemberName, typeof(string));
		info.AddValue("MMSignature", Signature, typeof(byte[]));
	}
}
