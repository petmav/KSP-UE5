using Unity;

namespace System.Drawing.Drawing2D;

public sealed class HatchBrush : Brush
{
	public Color BackgroundColor
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(Color);
		}
	}

	public Color ForegroundColor
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(Color);
		}
	}

	public HatchStyle HatchStyle
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(HatchStyle);
		}
	}

	public HatchBrush(HatchStyle hatchstyle, Color foreColor)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public HatchBrush(HatchStyle hatchstyle, Color foreColor, Color backColor)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public override object Clone()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}
}
