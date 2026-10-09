using System.Reflection;
using System.Runtime.InteropServices;
using Unity;

namespace System.EnterpriseServices.Internal;

[Guid("d8013ff1-730b-45e2-ba24-874b7242c425")]
public class GenerateMetadata : IComSoapMetadata
{
	public GenerateMetadata()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public string Generate(string strSrcTypeLib, string outPath)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public string GenerateMetaData(string strSrcTypeLib, string outPath, byte[] PublicKey, StrongNameKeyPair KeyPair)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public string GenerateSigned(string strSrcTypeLib, string outPath, bool InstallGac, out string Error)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	[DllImport("kernel32.dll", PreserveSig = false)]
	public static extern int SearchPath(string path, string fileName, string extension, int numBufferChars, string buffer, int[] filePart);
}
