using System;

namespace I18N.CJK;

[Serializable]
public class CP50221 : ISO2022JPEncoding
{
	public override string EncodingName => "Japanese (JIS-Allow 1 byte Kana)";

	public CP50221()
		: base(50221, allow1ByteKana: true, allowShiftIO: false)
	{
	}
}
