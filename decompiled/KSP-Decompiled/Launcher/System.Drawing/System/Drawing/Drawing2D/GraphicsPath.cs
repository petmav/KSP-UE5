using Unity;

namespace System.Drawing.Drawing2D;

public sealed class GraphicsPath : MarshalByRefObject, ICloneable, IDisposable
{
	public FillMode FillMode
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(FillMode);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public PathData PathData
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public PointF[] PathPoints
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public byte[] PathTypes
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
	}

	public int PointCount
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
	}

	public GraphicsPath()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public GraphicsPath(FillMode fillMode)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public GraphicsPath(PointF[] pts, byte[] types)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public GraphicsPath(PointF[] pts, byte[] types, FillMode fillMode)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public GraphicsPath(Point[] pts, byte[] types)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public GraphicsPath(Point[] pts, byte[] types, FillMode fillMode)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void AddArc(Rectangle rect, float startAngle, float sweepAngle)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void AddArc(RectangleF rect, float startAngle, float sweepAngle)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void AddArc(int x, int y, int width, int height, float startAngle, float sweepAngle)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void AddArc(float x, float y, float width, float height, float startAngle, float sweepAngle)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void AddBezier(Point pt1, Point pt2, Point pt3, Point pt4)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void AddBezier(PointF pt1, PointF pt2, PointF pt3, PointF pt4)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void AddBezier(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void AddBezier(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void AddBeziers(PointF[] points)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void AddBeziers(Point[] points)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void AddClosedCurve(PointF[] points)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void AddClosedCurve(PointF[] points, float tension)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void AddClosedCurve(Point[] points)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void AddClosedCurve(Point[] points, float tension)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void AddCurve(PointF[] points)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void AddCurve(PointF[] points, int offset, int numberOfSegments, float tension)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void AddCurve(PointF[] points, float tension)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void AddCurve(Point[] points)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void AddCurve(Point[] points, int offset, int numberOfSegments, float tension)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void AddCurve(Point[] points, float tension)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void AddEllipse(Rectangle rect)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void AddEllipse(RectangleF rect)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void AddEllipse(int x, int y, int width, int height)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void AddEllipse(float x, float y, float width, float height)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void AddLine(Point pt1, Point pt2)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void AddLine(PointF pt1, PointF pt2)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void AddLine(int x1, int y1, int x2, int y2)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void AddLine(float x1, float y1, float x2, float y2)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void AddLines(PointF[] points)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void AddLines(Point[] points)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void AddPath(GraphicsPath addingPath, bool connect)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void AddPie(Rectangle rect, float startAngle, float sweepAngle)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void AddPie(int x, int y, int width, int height, float startAngle, float sweepAngle)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void AddPie(float x, float y, float width, float height, float startAngle, float sweepAngle)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void AddPolygon(PointF[] points)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void AddPolygon(Point[] points)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void AddRectangle(Rectangle rect)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void AddRectangle(RectangleF rect)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void AddRectangles(RectangleF[] rects)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void AddRectangles(Rectangle[] rects)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void AddString(string s, FontFamily family, int style, float emSize, Point origin, StringFormat format)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void AddString(string s, FontFamily family, int style, float emSize, PointF origin, StringFormat format)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void AddString(string s, FontFamily family, int style, float emSize, Rectangle layoutRect, StringFormat format)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void AddString(string s, FontFamily family, int style, float emSize, RectangleF layoutRect, StringFormat format)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void ClearMarkers()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public object Clone()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public void CloseAllFigures()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void CloseFigure()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Dispose()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Flatten()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Flatten(Matrix matrix)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Flatten(Matrix matrix, float flatness)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public RectangleF GetBounds()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(RectangleF);
	}

	public RectangleF GetBounds(Matrix matrix)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(RectangleF);
	}

	public RectangleF GetBounds(Matrix matrix, Pen pen)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(RectangleF);
	}

	public PointF GetLastPoint()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(PointF);
	}

	public bool IsOutlineVisible(Point point, Pen pen)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public bool IsOutlineVisible(Point pt, Pen pen, Graphics graphics)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public bool IsOutlineVisible(PointF point, Pen pen)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public bool IsOutlineVisible(PointF pt, Pen pen, Graphics graphics)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public bool IsOutlineVisible(int x, int y, Pen pen)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public bool IsOutlineVisible(int x, int y, Pen pen, Graphics graphics)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public bool IsOutlineVisible(float x, float y, Pen pen)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public bool IsOutlineVisible(float x, float y, Pen pen, Graphics graphics)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public bool IsVisible(Point point)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public bool IsVisible(Point pt, Graphics graphics)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public bool IsVisible(PointF point)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public bool IsVisible(PointF pt, Graphics graphics)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public bool IsVisible(int x, int y)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public bool IsVisible(int x, int y, Graphics graphics)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public bool IsVisible(float x, float y)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public bool IsVisible(float x, float y, Graphics graphics)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public void Reset()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Reverse()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void SetMarkers()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void StartFigure()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Transform(Matrix matrix)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Warp(PointF[] destPoints, RectangleF srcRect)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Warp(PointF[] destPoints, RectangleF srcRect, Matrix matrix)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Warp(PointF[] destPoints, RectangleF srcRect, Matrix matrix, WarpMode warpMode)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Warp(PointF[] destPoints, RectangleF srcRect, Matrix matrix, WarpMode warpMode, float flatness)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Widen(Pen pen)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Widen(Pen pen, Matrix matrix)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Widen(Pen pen, Matrix matrix, float flatness)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
