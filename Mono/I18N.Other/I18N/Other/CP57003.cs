using System;

namespace I18N.Other;

[Serializable]
public class CP57003 : ISCIIEncoding
{
	public CP57003()
		: base(57003, 2432, "ISCII Bengali", "x-iscii-be")
	{
	}
}
