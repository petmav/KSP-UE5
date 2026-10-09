using System.Drawing.Drawing2D;
using Unity;

namespace System.Drawing;

public sealed class Pen : MarshalByRefObject, ICloneable, IDisposable
{
	public PenAlignment Alignment
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(PenAlignment);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public Brush Brush
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

	public float[] CompoundArray
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

	public CustomLineCap CustomEndCap
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

	public CustomLineCap CustomStartCap
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

	public DashCap DashCap
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(DashCap);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public float DashOffset
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

	public float[] DashPattern
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

	public DashStyle DashStyle
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(DashStyle);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public LineCap EndCap
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

	public LineJoin LineJoin
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

	public float MiterLimit
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

	public PenType PenType
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(PenType);
		}
	}

	public LineCap StartCap
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

	public Matrix Transform
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

	public float Width
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

	public Pen(Brush brush)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Pen(Brush brush, float width)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Pen(Color color)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Pen(Color color, float width)
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

	public void MultiplyTransform(Matrix matrix)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void MultiplyTransform(Matrix matrix, MatrixOrder order)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void ResetTransform()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void RotateTransform(float angle)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void RotateTransform(float angle, MatrixOrder order)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void ScaleTransform(float sx, float sy)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void ScaleTransform(float sx, float sy, MatrixOrder order)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void SetLineCap(LineCap startCap, LineCap endCap, DashCap dashCap)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void TranslateTransform(float dx, float dy)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void TranslateTransform(float dx, float dy, MatrixOrder order)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
