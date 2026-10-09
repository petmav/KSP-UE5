using Unity;

namespace System.Drawing.Printing;

[Serializable]
public class PaperSize
{
	public int Height
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

	public PaperKind Kind
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(PaperKind);
		}
	}

	public string PaperName
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

	public int Width
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

	public PaperSize()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public PaperSize(string name, int width, int height)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
