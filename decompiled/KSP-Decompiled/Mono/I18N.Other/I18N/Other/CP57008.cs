using System;

namespace I18N.Other;

[Serializable]
public class CP57008 : ISCIIEncoding
{
	public CP57008()
		: base(57008, 3200, "ISCII Kannada", "x-iscii-ka")
	{
	}
}
