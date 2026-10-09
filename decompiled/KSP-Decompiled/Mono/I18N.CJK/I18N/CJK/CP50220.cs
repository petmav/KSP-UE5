using System;

namespace I18N.CJK;

[Serializable]
public class CP50220 : ISO2022JPEncoding
{
	public override string EncodingName => "Japanese (JIS)";

	public CP50220()
		: base(50220, allow1ByteKana: false, allowShiftIO: false)
	{
	}
}
