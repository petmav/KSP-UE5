namespace System.Text;

[Serializable]
public sealed class EncoderReplacementFallback : EncoderFallback
{
	private string strDefault;

	public string DefaultString => strDefault;

	public override int MaxCharCount => strDefault.Length;

	public EncoderReplacementFallback()
		: this("?")
	{
	}

	public EncoderReplacementFallback(string replacement)
	{
		if (replacement == null)
		{
			throw new ArgumentNullException("replacement");
		}
		bool flag = false;
		for (int i = 0; i < replacement.Length; i++)
		{
			if (char.IsSurrogate(replacement, i))
			{
				if (char.IsHighSurrogate(replacement, i))
				{
					if (flag)
					{
						break;
					}
					flag = true;
					continue;
				}
				if (!flag)
				{
					flag = true;
					break;
				}
				flag = false;
			}
			else if (flag)
			{
				break;
			}
		}
		if (flag)
		{
			throw new ArgumentException(Environment.GetResourceString("String contains invalid Unicode code points.", "replacement"));
		}
		strDefault = replacement;
	}

	public override EncoderFallbackBuffer CreateFallbackBuffer()
	{
		return new EncoderReplacementFallbackBuffer(this);
	}

	public override bool Equals(object value)
	{
		if (value is EncoderReplacementFallback encoderReplacementFallback)
		{
			return strDefault == encoderReplacementFallback.strDefault;
		}
		return false;
	}

	public override int GetHashCode()
	{
		return strDefault.GetHashCode();
	}
}
