using Unity;

namespace System.Drawing;

public sealed class SolidBrush : Brush
{
	public Color Color
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(Color);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public SolidBrush(Color color)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public override object Clone()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	protected override void Dispose(bool disposing)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
