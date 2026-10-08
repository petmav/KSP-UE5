using System.Runtime.InteropServices;

namespace TDx.TDxInput
{
	[ComImport]
	[CoClass(typeof(TDxInfoClass))]
	[Guid("00612962-8FB6-47B2-BF98-4E8C0FF5F559")]
	public interface TDxInfo : ITDxInfo
	{
	}
}
