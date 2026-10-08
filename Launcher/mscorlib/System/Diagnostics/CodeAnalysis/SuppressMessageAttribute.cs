namespace System.Diagnostics.CodeAnalysis;

[AttributeUsage(AttributeTargets.All, Inherited = false, AllowMultiple = true)]
[Conditional("CODE_ANALYSIS")]
public sealed class SuppressMessageAttribute : Attribute
{
	private string category;

	private string justification;

	private string checkId;

	private string scope;

	private string target;

	private string messageId;

	public string Category => category;

	public string CheckId => checkId;

	public string Scope
	{
		get
		{
			return scope;
		}
		set
		{
			scope = value;
		}
	}

	public string Target
	{
		get
		{
			return target;
		}
		set
		{
			target = value;
		}
	}

	public string MessageId
	{
		get
		{
			return messageId;
		}
		set
		{
			messageId = value;
		}
	}

	public string Justification
	{
		get
		{
			return justification;
		}
		set
		{
			justification = value;
		}
	}

	public SuppressMessageAttribute(string category, string checkId)
	{
		this.category = category;
		this.checkId = checkId;
	}
}
