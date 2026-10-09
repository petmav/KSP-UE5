using System.Text;

namespace System.Net.Mail;

public class MailAddress
{
	private string address;

	private string displayName;

	private string host;

	private string user;

	private string to_string;

	public string Address => address;

	public string DisplayName
	{
		get
		{
			if (displayName == null)
			{
				return string.Empty;
			}
			return displayName;
		}
	}

	public string Host => host;

	public string User => user;

	public MailAddress(string address)
		: this(address, null)
	{
	}

	public MailAddress(string address, string displayName)
		: this(address, displayName, Encoding.UTF8)
	{
	}

	[System.MonoTODO("We don't do anything with displayNameEncoding")]
	public MailAddress(string address, string displayName, Encoding displayNameEncoding)
	{
		if (address == null)
		{
			throw new ArgumentNullException("address");
		}
		if (address.Length == 0)
		{
			throw new ArgumentException("address");
		}
		if (displayName != null)
		{
			this.displayName = displayName.Trim();
		}
		ParseAddress(address);
	}

	private void ParseAddress(string address)
	{
		address = address.Trim();
		int num = address.IndexOf('"');
		if (num != -1)
		{
			if (num != 0 || address.Length == 1)
			{
				throw CreateFormatException();
			}
			int num2 = address.LastIndexOf('"');
			if (num2 == num)
			{
				throw CreateFormatException();
			}
			if (displayName == null)
			{
				displayName = address.Substring(num + 1, num2 - num - 1).Trim();
			}
			address = address.Substring(num2 + 1).Trim();
		}
		num = address.IndexOf('<');
		if (num >= 0)
		{
			if (displayName == null)
			{
				displayName = address.Substring(0, num).Trim();
			}
			if (address.Length - 1 == num)
			{
				throw CreateFormatException();
			}
			int num3 = address.IndexOf('>', num + 1);
			if (num3 == -1)
			{
				throw CreateFormatException();
			}
			address = address.Substring(num + 1, num3 - num - 1).Trim();
		}
		this.address = address;
		num = address.IndexOf('@');
		if (num <= 0)
		{
			throw CreateFormatException();
		}
		if (num != address.LastIndexOf('@'))
		{
			throw CreateFormatException();
		}
		user = address.Substring(0, num).Trim();
		if (user.Length == 0)
		{
			throw CreateFormatException();
		}
		host = address.Substring(num + 1).Trim();
		if (host.Length == 0)
		{
			throw CreateFormatException();
		}
	}

	public override bool Equals(object value)
	{
		if (value == null)
		{
			return false;
		}
		return string.Compare(ToString(), value.ToString(), StringComparison.OrdinalIgnoreCase) == 0;
	}

	public override int GetHashCode()
	{
		return ToString().GetHashCode();
	}

	public override string ToString()
	{
		if (to_string != null)
		{
			return to_string;
		}
		if (!string.IsNullOrEmpty(displayName))
		{
			to_string = $"\"{DisplayName}\" <{Address}>";
		}
		else
		{
			to_string = address;
		}
		return to_string;
	}

	private static FormatException CreateFormatException()
	{
		return new FormatException("The specified string is not in the form required for an e-mail address.");
	}
}
