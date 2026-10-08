using Unity;

namespace System.Drawing.Drawing2D;

public sealed class Matrix : MarshalByRefObject, IDisposable
{
	public float[] Elements
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public bool IsIdentity
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	public bool IsInvertible
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	public float OffsetX
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(float);
		}
	}

	public float OffsetY
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(float);
		}
	}

	public Matrix()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Matrix(Rectangle rect, Point[] plgpts)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Matrix(RectangleF rect, PointF[] plgpts)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Matrix(float m11, float m12, float m21, float m22, float dx, float dy)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Matrix Clone()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public void Dispose()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Invert()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Multiply(Matrix matrix)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Multiply(Matrix matrix, MatrixOrder order)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Reset()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Rotate(float angle)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Rotate(float angle, MatrixOrder order)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void RotateAt(float angle, PointF point)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void RotateAt(float angle, PointF point, MatrixOrder order)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Scale(float scaleX, float scaleY)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Scale(float scaleX, float scaleY, MatrixOrder order)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Shear(float shearX, float shearY)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Shear(float shearX, float shearY, MatrixOrder order)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void TransformPoints(PointF[] pts)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void TransformPoints(Point[] pts)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void TransformVectors(PointF[] pts)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void TransformVectors(Point[] pts)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Translate(float offsetX, float offsetY)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Translate(float offsetX, float offsetY, MatrixOrder order)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void VectorTransformPoints(Point[] pts)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
