using Unity;

namespace System.Drawing.Printing;

public class PrintPageEventArgs : EventArgs
{
	public bool Cancel
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public Graphics Graphics
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public bool HasMorePages
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public Rectangle MarginBounds
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(Rectangle);
		}
	}

	public Rectangle PageBounds
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(Rectangle);
		}
	}

	public PageSettings PageSettings
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public PrintPageEventArgs(Graphics graphics, Rectangle marginBounds, Rectangle pageBounds, PageSettings pageSettings)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
