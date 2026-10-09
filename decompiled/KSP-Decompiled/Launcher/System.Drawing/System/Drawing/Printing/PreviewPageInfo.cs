using Unity;

namespace System.Drawing.Printing;

public sealed class PreviewPageInfo
{
	public Image Image
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public Size PhysicalSize
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(Size);
		}
	}

	public PreviewPageInfo(Image image, Size physicalSize)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
