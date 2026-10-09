using System;

namespace I18N.CJK;

[Serializable]
public class CP50222 : ISO2022JPEncoding
{
	public override string EncodingName => "Japanese (JIS-Allow 1 byte Kana - SO/SI)";

	public CP50222()
		: base(50222, allow1ByteKana: true, allowShiftIO: true)
	{
	}
}
