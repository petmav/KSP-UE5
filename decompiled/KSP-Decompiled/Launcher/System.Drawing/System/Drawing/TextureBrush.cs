using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
using Unity;

namespace System.Drawing;

public sealed class TextureBrush : Brush
{
	public Image Image
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
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

	public WrapMode WrapMode
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(WrapMode);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public TextureBrush(Image bitmap)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public TextureBrush(Image image, WrapMode wrapMode)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public TextureBrush(Image image, WrapMode wrapMode, Rectangle dstRect)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public TextureBrush(Image image, WrapMode wrapMode, RectangleF dstRect)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public TextureBrush(Image image, Rectangle dstRect)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public TextureBrush(Image image, Rectangle dstRect, ImageAttributes imageAttr)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public TextureBrush(Image image, RectangleF dstRect)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public TextureBrush(Image image, RectangleF dstRect, ImageAttributes imageAttr)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public override object Clone()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
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

	public void TranslateTransform(float dx, float dy)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void TranslateTransform(float dx, float dy, MatrixOrder order)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
