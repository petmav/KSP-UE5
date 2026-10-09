using System.Security.Permissions;
using Unity;

namespace System.Media;

[HostProtection(SecurityAction.LinkDemand, UI = true)]
public sealed class SystemSounds
{
	public static SystemSound Asterisk
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public static SystemSound Beep
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public static SystemSound Exclamation
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public static SystemSound Hand
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public static SystemSound Question
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	internal SystemSounds()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
