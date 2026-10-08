using Unity;

namespace System.Drawing.Printing;

[Serializable]
public class PaperSource
{
	public PaperSourceKind Kind
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(PaperSourceKind);
		}
	}

	public int RawKind
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

	public string SourceName
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

	public PaperSource()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
