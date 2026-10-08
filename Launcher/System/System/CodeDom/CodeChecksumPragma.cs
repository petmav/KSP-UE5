using System.Runtime.InteropServices;
using Unity;

namespace System.CodeDom;

[Serializable]
[ClassInterface(ClassInterfaceType.AutoDispatch)]
[ComVisible(true)]
public class CodeChecksumPragma : CodeDirective
{
	public Guid ChecksumAlgorithmId
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(Guid);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public byte[] ChecksumData
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public string FileName
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public CodeChecksumPragma()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeChecksumPragma(string fileName, Guid checksumAlgorithmId, byte[] checksumData)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
