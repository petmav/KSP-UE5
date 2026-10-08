using System;

namespace I18N.CJK;

[Serializable]
internal class CP949 : KoreanEncoding
{
	private const int UHC_CODE_PAGE = 949;

	public override string BodyName => "ks_c_5601-1987";

	public override string EncodingName => "Korean (UHC)";

	public override string HeaderName => "ks_c_5601-1987";

	public override string WebName => "ks_c_5601-1987";

	public CP949()
		: base(949, useUHC: true)
	{
	}
}
