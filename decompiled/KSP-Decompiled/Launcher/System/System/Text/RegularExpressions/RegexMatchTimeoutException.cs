using System.Runtime.Serialization;
using System.Security.Permissions;

namespace System.Text.RegularExpressions;

[Serializable]
public class RegexMatchTimeoutException : TimeoutException, ISerializable
{
	private string regexInput;

	private string regexPattern;

	private TimeSpan matchTimeout = TimeSpan.FromTicks(-1L);

	public string Pattern
	{
		[PermissionSet(SecurityAction.LinkDemand, Unrestricted = true)]
		get
		{
			return regexPattern;
		}
	}

	public string Input
	{
		[PermissionSet(SecurityAction.LinkDemand, Unrestricted = true)]
		get
		{
			return regexInput;
		}
	}

	public TimeSpan MatchTimeout
	{
		[PermissionSet(SecurityAction.LinkDemand, Unrestricted = true)]
		get
		{
			return matchTimeout;
		}
	}

	public RegexMatchTimeoutException(string regexInput, string regexPattern, TimeSpan matchTimeout)
		: base(global::SR.GetString("The RegEx engine has timed out while trying to match a pattern to an input string. This can occur for many reasons, including very large inputs or excessive backtracking caused by nested quantifiers, back-references and other factors."))
	{
		Init(regexInput, regexPattern, matchTimeout);
	}

	public RegexMatchTimeoutException()
	{
		Init();
	}

	public RegexMatchTimeoutException(string message)
		: base(message)
	{
		Init();
	}

	public RegexMatchTimeoutException(string message, Exception inner)
		: base(message, inner)
	{
		Init();
	}

	[SecurityPermission(SecurityAction.LinkDemand, SerializationFormatter = true)]
	protected RegexMatchTimeoutException(SerializationInfo info, StreamingContext context)
		: base(info, context)
	{
		string @string = info.GetString("regexInput");
		string string2 = info.GetString("regexPattern");
		TimeSpan timeout = TimeSpan.FromTicks(info.GetInt64("timeoutTicks"));
		Init(@string, string2, timeout);
	}

	[SecurityPermission(SecurityAction.LinkDemand, SerializationFormatter = true)]
	void ISerializable.GetObjectData(SerializationInfo si, StreamingContext context)
	{
		base.GetObjectData(si, context);
		si.AddValue("regexInput", regexInput);
		si.AddValue("regexPattern", regexPattern);
		si.AddValue("timeoutTicks", matchTimeout.Ticks);
	}

	private void Init()
	{
		Init("", "", TimeSpan.FromTicks(-1L));
	}

	private void Init(string input, string pattern, TimeSpan timeout)
	{
		regexInput = input;
		regexPattern = pattern;
		matchTimeout = timeout;
	}
}
