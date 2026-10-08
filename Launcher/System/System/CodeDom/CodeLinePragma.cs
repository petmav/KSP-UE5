using System.Runtime.InteropServices;
using Unity;

namespace System.CodeDom;

[Serializable]
[ClassInterface(ClassInterfaceType.AutoDispatch)]
[ComVisible(true)]
public class CodeLinePragma
{
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

	public int LineNumber
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public CodeLinePragma()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CodeLinePragma(string fileName, int lineNumber)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
