using System.Runtime.CompilerServices;
using System.Runtime.Serialization;
using System.Security.Permissions;

namespace System;

[Serializable]
[TypeForwardedFrom("System.Core, Version=2.0.5.0, Culture=Neutral, PublicKeyToken=7cec85d7bea7798e")]
[HostProtection(SecurityAction.LinkDemand, MayLeakOnAbort = true)]
public class TimeZoneNotFoundException : Exception
{
	public TimeZoneNotFoundException(string message)
		: base(message)
	{
	}

	public TimeZoneNotFoundException(string message, Exception innerException)
		: base(message, innerException)
	{
	}

	protected TimeZoneNotFoundException(SerializationInfo info, StreamingContext context)
		: base(info, context)
	{
	}

	public TimeZoneNotFoundException()
	{
	}
}
