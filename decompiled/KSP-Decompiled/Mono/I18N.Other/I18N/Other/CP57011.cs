using System;

namespace I18N.Other;

[Serializable]
public class CP57011 : ISCIIEncoding
{
	public CP57011()
		: base(57011, 2560, "ISCII Punjabi", "x-iscii-pa")
	{
	}
}
