using Unity;

namespace System.Drawing.Drawing2D;

public class CustomLineCap : MarshalByRefObject, ICloneable, IDisposable
{
	public LineCap BaseCap
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(LineCap);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public float BaseInset
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(float);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public LineJoin StrokeJoin
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(LineJoin);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public float WidthScale
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(float);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public CustomLineCap(GraphicsPath fillPath, GraphicsPath strokePath)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CustomLineCap(GraphicsPath fillPath, GraphicsPath strokePath, LineCap baseCap)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public CustomLineCap(GraphicsPath fillPath, GraphicsPath strokePath, LineCap baseCap, float baseInset)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public object Clone()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public void Dispose()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected virtual void Dispose(bool disposing)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void GetStrokeCaps(out LineCap startCap, out LineCap endCap)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void SetStrokeCaps(LineCap startCap, LineCap endCap)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
