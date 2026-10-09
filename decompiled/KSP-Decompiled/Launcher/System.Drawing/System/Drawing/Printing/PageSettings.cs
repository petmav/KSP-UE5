using Unity;

namespace System.Drawing.Printing;

[Serializable]
public class PageSettings : ICloneable
{
	public Rectangle Bounds
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(Rectangle);
		}
	}

	public bool Color
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

	public float HardMarginX
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(float);
		}
	}

	public float HardMarginY
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(float);
		}
	}

	public bool Landscape
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

	public Margins Margins
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

	public PaperSize PaperSize
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

	public PaperSource PaperSource
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

	public RectangleF PrintableArea
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(RectangleF);
		}
	}

	public PrinterResolution PrinterResolution
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

	public PrinterSettings PrinterSettings
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

	public PageSettings()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public PageSettings(PrinterSettings printerSettings)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public object Clone()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public void CopyToHdevmode(IntPtr hdevmode)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void SetHdevmode(IntPtr hdevmode)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
