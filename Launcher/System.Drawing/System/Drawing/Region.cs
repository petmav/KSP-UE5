using System.Drawing.Drawing2D;
using Unity;

namespace System.Drawing;

public sealed class Region : MarshalByRefObject, IDisposable
{
	public Region()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Region(GraphicsPath path)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Region(RegionData rgnData)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Region(Rectangle rect)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Region(RectangleF rect)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Region Clone()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public void Complement(GraphicsPath path)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Complement(Rectangle rect)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Complement(RectangleF rect)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Complement(Region region)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Dispose()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public bool Equals(Region region, Graphics g)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public void Exclude(GraphicsPath path)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Exclude(Rectangle rect)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Exclude(RectangleF rect)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Exclude(Region region)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public static Region FromHrgn(IntPtr hrgn)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public RectangleF GetBounds(Graphics g)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(RectangleF);
	}

	public IntPtr GetHrgn(Graphics g)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(IntPtr);
	}

	public RegionData GetRegionData()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public RectangleF[] GetRegionScans(Matrix matrix)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public void Intersect(GraphicsPath path)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Intersect(Rectangle rect)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Intersect(RectangleF rect)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Intersect(Region region)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public bool IsEmpty(Graphics g)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public bool IsInfinite(Graphics g)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public bool IsVisible(Point point)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public bool IsVisible(Point point, Graphics g)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public bool IsVisible(PointF point)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public bool IsVisible(PointF point, Graphics g)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public bool IsVisible(Rectangle rect)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public bool IsVisible(Rectangle rect, Graphics g)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public bool IsVisible(RectangleF rect)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public bool IsVisible(RectangleF rect, Graphics g)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public bool IsVisible(int x, int y, Graphics g)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public bool IsVisible(int x, int y, int width, int height)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public bool IsVisible(int x, int y, int width, int height, Graphics g)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public bool IsVisible(float x, float y)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public bool IsVisible(float x, float y, Graphics g)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public bool IsVisible(float x, float y, float width, float height)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public bool IsVisible(float x, float y, float width, float height, Graphics g)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public void MakeEmpty()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void MakeInfinite()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void ReleaseHrgn(IntPtr regionHandle)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Transform(Matrix matrix)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Translate(int dx, int dy)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Translate(float dx, float dy)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Union(GraphicsPath path)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Union(Rectangle rect)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Union(RectangleF rect)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Union(Region region)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Xor(GraphicsPath path)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Xor(Rectangle rect)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Xor(RectangleF rect)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Xor(Region region)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
