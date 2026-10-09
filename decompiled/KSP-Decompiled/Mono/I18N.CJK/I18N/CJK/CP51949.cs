using System;

namespace I18N.CJK;

[Serializable]
internal class CP51949 : KoreanEncoding
{
	private const int EUCKR_CODE_PAGE = 51949;

	public override string BodyName => "euc-kr";

	public override string EncodingName => "Korean (EUC)";

	public override string HeaderName => "euc-kr";

	public override string WebName => "euc-kr";

	public CP51949()
		: base(51949, useUHC: false)
	{
	}
}
