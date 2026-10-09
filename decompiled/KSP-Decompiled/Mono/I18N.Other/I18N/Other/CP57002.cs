using System;

namespace I18N.Other;

[Serializable]
public class CP57002 : ISCIIEncoding
{
	public CP57002()
		: base(57002, 2304, "ISCII Devanagari", "x-iscii-de")
	{
	}
}
