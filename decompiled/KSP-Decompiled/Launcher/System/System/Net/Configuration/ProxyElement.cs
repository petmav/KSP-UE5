using System.Configuration;
using Unity;

namespace System.Net.Configuration;

public sealed class ProxyElement : ConfigurationElement
{
	public enum AutoDetectValues
	{
		False = 0,
		True = 1,
		Unspecified = -1
	}

	public enum BypassOnLocalValues
	{
		False = 0,
		True = 1,
		Unspecified = -1
	}

	public enum UseSystemDefaultValues
	{
		False = 0,
		True = 1,
		Unspecified = -1
	}

	public AutoDetectValues AutoDetect
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(AutoDetectValues);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public BypassOnLocalValues BypassOnLocal
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(BypassOnLocalValues);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	protected override ConfigurationPropertyCollection Properties
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public Uri ProxyAddress
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public Uri ScriptLocation
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public UseSystemDefaultValues UseSystemDefault
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(UseSystemDefaultValues);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public ProxyElement()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
