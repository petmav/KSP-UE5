using System.ComponentModel;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
using System.Drawing.Text;
using System.Security.Permissions;
using Unity;

namespace System.Drawing;

public sealed class Graphics : MarshalByRefObject, IDeviceContext, IDisposable
{
	public delegate bool DrawImageAbort(IntPtr callbackdata);

	public delegate bool EnumerateMetafileProc(EmfPlusRecordType recordType, int flags, int dataSize, IntPtr data, PlayRecordCallback callbackData);

	public Region Clip
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

	public RectangleF ClipBounds
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(RectangleF);
		}
	}

	public CompositingMode CompositingMode
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(CompositingMode);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public CompositingQuality CompositingQuality
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(CompositingQuality);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public float DpiX
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(float);
		}
	}

	public float DpiY
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(float);
		}
	}

	public InterpolationMode InterpolationMode
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(InterpolationMode);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public bool IsClipEmpty
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	public bool IsVisibleClipEmpty
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	public float PageScale
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

	public GraphicsUnit PageUnit
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(GraphicsUnit);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public PixelOffsetMode PixelOffsetMode
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(PixelOffsetMode);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public Point RenderingOrigin
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(Point);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public SmoothingMode SmoothingMode
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(SmoothingMode);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public int TextContrast
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public TextRenderingHint TextRenderingHint
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(TextRenderingHint);
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

	public RectangleF VisibleClipBounds
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(RectangleF);
		}
	}

	internal Graphics()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void AddMetafileComment(byte[] data)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public GraphicsContainer BeginContainer()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public GraphicsContainer BeginContainer(Rectangle dstrect, Rectangle srcrect, GraphicsUnit unit)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public GraphicsContainer BeginContainer(RectangleF dstrect, RectangleF srcrect, GraphicsUnit unit)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public void Clear(Color color)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void CopyFromScreen(Point upperLeftSource, Point upperLeftDestination, Size blockRegionSize)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void CopyFromScreen(Point upperLeftSource, Point upperLeftDestination, Size blockRegionSize, CopyPixelOperation copyPixelOperation)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void CopyFromScreen(int sourceX, int sourceY, int destinationX, int destinationY, Size blockRegionSize)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void CopyFromScreen(int sourceX, int sourceY, int destinationX, int destinationY, Size blockRegionSize, CopyPixelOperation copyPixelOperation)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Dispose()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawArc(Pen pen, Rectangle rect, float startAngle, float sweepAngle)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawArc(Pen pen, RectangleF rect, float startAngle, float sweepAngle)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawArc(Pen pen, int x, int y, int width, int height, int startAngle, int sweepAngle)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawArc(Pen pen, float x, float y, float width, float height, float startAngle, float sweepAngle)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawBezier(Pen pen, Point pt1, Point pt2, Point pt3, Point pt4)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawBezier(Pen pen, PointF pt1, PointF pt2, PointF pt3, PointF pt4)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawBezier(Pen pen, float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawBeziers(Pen pen, PointF[] points)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawBeziers(Pen pen, Point[] points)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawClosedCurve(Pen pen, PointF[] points)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawClosedCurve(Pen pen, PointF[] points, float tension, FillMode fillmode)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawClosedCurve(Pen pen, Point[] points)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawClosedCurve(Pen pen, Point[] points, float tension, FillMode fillmode)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawCurve(Pen pen, PointF[] points)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawCurve(Pen pen, PointF[] points, int offset, int numberOfSegments)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawCurve(Pen pen, PointF[] points, int offset, int numberOfSegments, float tension)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawCurve(Pen pen, PointF[] points, float tension)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawCurve(Pen pen, Point[] points)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawCurve(Pen pen, Point[] points, int offset, int numberOfSegments, float tension)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawCurve(Pen pen, Point[] points, float tension)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawEllipse(Pen pen, Rectangle rect)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawEllipse(Pen pen, RectangleF rect)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawEllipse(Pen pen, int x, int y, int width, int height)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawEllipse(Pen pen, float x, float y, float width, float height)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawIcon(Icon icon, Rectangle targetRect)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawIcon(Icon icon, int x, int y)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawIconUnstretched(Icon icon, Rectangle targetRect)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawImage(Image image, Point point)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawImage(Image image, PointF point)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawImage(Image image, PointF[] destPoints)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawImage(Image image, PointF[] destPoints, RectangleF srcRect, GraphicsUnit srcUnit)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawImage(Image image, PointF[] destPoints, RectangleF srcRect, GraphicsUnit srcUnit, ImageAttributes imageAttr)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawImage(Image image, PointF[] destPoints, RectangleF srcRect, GraphicsUnit srcUnit, ImageAttributes imageAttr, DrawImageAbort callback)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawImage(Image image, PointF[] destPoints, RectangleF srcRect, GraphicsUnit srcUnit, ImageAttributes imageAttr, DrawImageAbort callback, int callbackData)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawImage(Image image, Point[] destPoints)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawImage(Image image, Point[] destPoints, Rectangle srcRect, GraphicsUnit srcUnit)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawImage(Image image, Point[] destPoints, Rectangle srcRect, GraphicsUnit srcUnit, ImageAttributes imageAttr)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawImage(Image image, Point[] destPoints, Rectangle srcRect, GraphicsUnit srcUnit, ImageAttributes imageAttr, DrawImageAbort callback)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawImage(Image image, Point[] destPoints, Rectangle srcRect, GraphicsUnit srcUnit, ImageAttributes imageAttr, DrawImageAbort callback, int callbackData)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawImage(Image image, Rectangle rect)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawImage(Image image, Rectangle destRect, Rectangle srcRect, GraphicsUnit srcUnit)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawImage(Image image, Rectangle destRect, int srcX, int srcY, int srcWidth, int srcHeight, GraphicsUnit srcUnit)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawImage(Image image, Rectangle destRect, int srcX, int srcY, int srcWidth, int srcHeight, GraphicsUnit srcUnit, ImageAttributes imageAttr)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawImage(Image image, Rectangle destRect, int srcX, int srcY, int srcWidth, int srcHeight, GraphicsUnit srcUnit, ImageAttributes imageAttr, DrawImageAbort callback)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawImage(Image image, Rectangle destRect, int srcX, int srcY, int srcWidth, int srcHeight, GraphicsUnit srcUnit, ImageAttributes imageAttrs, DrawImageAbort callback, IntPtr callbackData)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawImage(Image image, Rectangle destRect, float srcX, float srcY, float srcWidth, float srcHeight, GraphicsUnit srcUnit)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawImage(Image image, Rectangle destRect, float srcX, float srcY, float srcWidth, float srcHeight, GraphicsUnit srcUnit, ImageAttributes imageAttrs)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawImage(Image image, Rectangle destRect, float srcX, float srcY, float srcWidth, float srcHeight, GraphicsUnit srcUnit, ImageAttributes imageAttrs, DrawImageAbort callback)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawImage(Image image, Rectangle destRect, float srcX, float srcY, float srcWidth, float srcHeight, GraphicsUnit srcUnit, ImageAttributes imageAttrs, DrawImageAbort callback, IntPtr callbackData)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawImage(Image image, RectangleF rect)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawImage(Image image, RectangleF destRect, RectangleF srcRect, GraphicsUnit srcUnit)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawImage(Image image, int x, int y)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawImage(Image image, int x, int y, Rectangle srcRect, GraphicsUnit srcUnit)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawImage(Image image, int x, int y, int width, int height)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawImage(Image image, float x, float y)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawImage(Image image, float x, float y, RectangleF srcRect, GraphicsUnit srcUnit)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawImage(Image image, float x, float y, float width, float height)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawImageUnscaled(Image image, Point point)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawImageUnscaled(Image image, Rectangle rect)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawImageUnscaled(Image image, int x, int y)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawImageUnscaled(Image image, int x, int y, int width, int height)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawImageUnscaledAndClipped(Image image, Rectangle rect)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawLine(Pen pen, Point pt1, Point pt2)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawLine(Pen pen, PointF pt1, PointF pt2)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawLine(Pen pen, int x1, int y1, int x2, int y2)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawLine(Pen pen, float x1, float y1, float x2, float y2)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawLines(Pen pen, PointF[] points)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawLines(Pen pen, Point[] points)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawPath(Pen pen, GraphicsPath path)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawPie(Pen pen, Rectangle rect, float startAngle, float sweepAngle)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawPie(Pen pen, RectangleF rect, float startAngle, float sweepAngle)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawPie(Pen pen, int x, int y, int width, int height, int startAngle, int sweepAngle)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawPie(Pen pen, float x, float y, float width, float height, float startAngle, float sweepAngle)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawPolygon(Pen pen, PointF[] points)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawPolygon(Pen pen, Point[] points)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawRectangle(Pen pen, Rectangle rect)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawRectangle(Pen pen, int x, int y, int width, int height)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawRectangle(Pen pen, float x, float y, float width, float height)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawRectangles(Pen pen, RectangleF[] rects)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawRectangles(Pen pen, Rectangle[] rects)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawString(string s, Font font, Brush brush, PointF point)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawString(string s, Font font, Brush brush, PointF point, StringFormat format)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawString(string s, Font font, Brush brush, RectangleF layoutRectangle)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawString(string s, Font font, Brush brush, RectangleF layoutRectangle, StringFormat format)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawString(string s, Font font, Brush brush, float x, float y)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void DrawString(string s, Font font, Brush brush, float x, float y, StringFormat format)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void EndContainer(GraphicsContainer container)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void EnumerateMetafile(Metafile metafile, Point destPoint, EnumerateMetafileProc callback)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void EnumerateMetafile(Metafile metafile, Point destPoint, EnumerateMetafileProc callback, IntPtr callbackData)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void EnumerateMetafile(Metafile metafile, Point destPoint, EnumerateMetafileProc callback, IntPtr callbackData, ImageAttributes imageAttr)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void EnumerateMetafile(Metafile metafile, Point destPoint, Rectangle srcRect, GraphicsUnit srcUnit, EnumerateMetafileProc callback)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void EnumerateMetafile(Metafile metafile, Point destPoint, Rectangle srcRect, GraphicsUnit srcUnit, EnumerateMetafileProc callback, IntPtr callbackData)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void EnumerateMetafile(Metafile metafile, Point destPoint, Rectangle srcRect, GraphicsUnit unit, EnumerateMetafileProc callback, IntPtr callbackData, ImageAttributes imageAttr)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void EnumerateMetafile(Metafile metafile, PointF destPoint, EnumerateMetafileProc callback)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void EnumerateMetafile(Metafile metafile, PointF destPoint, EnumerateMetafileProc callback, IntPtr callbackData)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void EnumerateMetafile(Metafile metafile, PointF destPoint, EnumerateMetafileProc callback, IntPtr callbackData, ImageAttributes imageAttr)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void EnumerateMetafile(Metafile metafile, PointF destPoint, RectangleF srcRect, GraphicsUnit srcUnit, EnumerateMetafileProc callback)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void EnumerateMetafile(Metafile metafile, PointF destPoint, RectangleF srcRect, GraphicsUnit srcUnit, EnumerateMetafileProc callback, IntPtr callbackData)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void EnumerateMetafile(Metafile metafile, PointF destPoint, RectangleF srcRect, GraphicsUnit unit, EnumerateMetafileProc callback, IntPtr callbackData, ImageAttributes imageAttr)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void EnumerateMetafile(Metafile metafile, PointF[] destPoints, EnumerateMetafileProc callback)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void EnumerateMetafile(Metafile metafile, PointF[] destPoints, EnumerateMetafileProc callback, IntPtr callbackData)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void EnumerateMetafile(Metafile metafile, PointF[] destPoints, EnumerateMetafileProc callback, IntPtr callbackData, ImageAttributes imageAttr)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void EnumerateMetafile(Metafile metafile, PointF[] destPoints, RectangleF srcRect, GraphicsUnit srcUnit, EnumerateMetafileProc callback)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void EnumerateMetafile(Metafile metafile, PointF[] destPoints, RectangleF srcRect, GraphicsUnit srcUnit, EnumerateMetafileProc callback, IntPtr callbackData)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void EnumerateMetafile(Metafile metafile, PointF[] destPoints, RectangleF srcRect, GraphicsUnit unit, EnumerateMetafileProc callback, IntPtr callbackData, ImageAttributes imageAttr)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void EnumerateMetafile(Metafile metafile, Point[] destPoints, EnumerateMetafileProc callback)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void EnumerateMetafile(Metafile metafile, Point[] destPoints, EnumerateMetafileProc callback, IntPtr callbackData)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void EnumerateMetafile(Metafile metafile, Point[] destPoints, EnumerateMetafileProc callback, IntPtr callbackData, ImageAttributes imageAttr)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void EnumerateMetafile(Metafile metafile, Point[] destPoints, Rectangle srcRect, GraphicsUnit srcUnit, EnumerateMetafileProc callback)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void EnumerateMetafile(Metafile metafile, Point[] destPoints, Rectangle srcRect, GraphicsUnit srcUnit, EnumerateMetafileProc callback, IntPtr callbackData)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void EnumerateMetafile(Metafile metafile, Point[] destPoints, Rectangle srcRect, GraphicsUnit unit, EnumerateMetafileProc callback, IntPtr callbackData, ImageAttributes imageAttr)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void EnumerateMetafile(Metafile metafile, Rectangle destRect, EnumerateMetafileProc callback)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void EnumerateMetafile(Metafile metafile, Rectangle destRect, EnumerateMetafileProc callback, IntPtr callbackData)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void EnumerateMetafile(Metafile metafile, Rectangle destRect, EnumerateMetafileProc callback, IntPtr callbackData, ImageAttributes imageAttr)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void EnumerateMetafile(Metafile metafile, Rectangle destRect, Rectangle srcRect, GraphicsUnit srcUnit, EnumerateMetafileProc callback)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void EnumerateMetafile(Metafile metafile, Rectangle destRect, Rectangle srcRect, GraphicsUnit srcUnit, EnumerateMetafileProc callback, IntPtr callbackData)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void EnumerateMetafile(Metafile metafile, Rectangle destRect, Rectangle srcRect, GraphicsUnit unit, EnumerateMetafileProc callback, IntPtr callbackData, ImageAttributes imageAttr)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void EnumerateMetafile(Metafile metafile, RectangleF destRect, EnumerateMetafileProc callback)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void EnumerateMetafile(Metafile metafile, RectangleF destRect, EnumerateMetafileProc callback, IntPtr callbackData)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void EnumerateMetafile(Metafile metafile, RectangleF destRect, EnumerateMetafileProc callback, IntPtr callbackData, ImageAttributes imageAttr)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void EnumerateMetafile(Metafile metafile, RectangleF destRect, RectangleF srcRect, GraphicsUnit srcUnit, EnumerateMetafileProc callback)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void EnumerateMetafile(Metafile metafile, RectangleF destRect, RectangleF srcRect, GraphicsUnit srcUnit, EnumerateMetafileProc callback, IntPtr callbackData)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void EnumerateMetafile(Metafile metafile, RectangleF destRect, RectangleF srcRect, GraphicsUnit unit, EnumerateMetafileProc callback, IntPtr callbackData, ImageAttributes imageAttr)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void ExcludeClip(Rectangle rect)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void ExcludeClip(Region region)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void FillClosedCurve(Brush brush, PointF[] points)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void FillClosedCurve(Brush brush, PointF[] points, FillMode fillmode)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void FillClosedCurve(Brush brush, PointF[] points, FillMode fillmode, float tension)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void FillClosedCurve(Brush brush, Point[] points)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void FillClosedCurve(Brush brush, Point[] points, FillMode fillmode)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void FillClosedCurve(Brush brush, Point[] points, FillMode fillmode, float tension)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void FillEllipse(Brush brush, Rectangle rect)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void FillEllipse(Brush brush, RectangleF rect)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void FillEllipse(Brush brush, int x, int y, int width, int height)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void FillEllipse(Brush brush, float x, float y, float width, float height)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void FillPath(Brush brush, GraphicsPath path)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void FillPie(Brush brush, Rectangle rect, float startAngle, float sweepAngle)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void FillPie(Brush brush, int x, int y, int width, int height, int startAngle, int sweepAngle)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void FillPie(Brush brush, float x, float y, float width, float height, float startAngle, float sweepAngle)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void FillPolygon(Brush brush, PointF[] points)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void FillPolygon(Brush brush, PointF[] points, FillMode fillMode)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void FillPolygon(Brush brush, Point[] points)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void FillPolygon(Brush brush, Point[] points, FillMode fillMode)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void FillRectangle(Brush brush, Rectangle rect)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void FillRectangle(Brush brush, RectangleF rect)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void FillRectangle(Brush brush, int x, int y, int width, int height)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void FillRectangle(Brush brush, float x, float y, float width, float height)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void FillRectangles(Brush brush, RectangleF[] rects)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void FillRectangles(Brush brush, Rectangle[] rects)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void FillRegion(Brush brush, Region region)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Flush()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Flush(FlushIntention intention)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	[EditorBrowsable(EditorBrowsableState.Advanced)]
	public static Graphics FromHdc(IntPtr hdc)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	[EditorBrowsable(EditorBrowsableState.Advanced)]
	public static Graphics FromHdc(IntPtr hdc, IntPtr hdevice)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	[EditorBrowsable(EditorBrowsableState.Advanced)]
	[SecurityPermission(SecurityAction.LinkDemand, Flags = SecurityPermissionFlag.UnmanagedCode)]
	public static Graphics FromHdcInternal(IntPtr hdc)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	[EditorBrowsable(EditorBrowsableState.Advanced)]
	public static Graphics FromHwnd(IntPtr hwnd)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	[EditorBrowsable(EditorBrowsableState.Advanced)]
	[SecurityPermission(SecurityAction.LinkDemand, Flags = SecurityPermissionFlag.UnmanagedCode)]
	public static Graphics FromHwndInternal(IntPtr hwnd)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public static Graphics FromImage(Image image)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	[EditorBrowsable(EditorBrowsableState.Never)]
	[StrongNameIdentityPermission(SecurityAction.LinkDemand, Name = "System.Windows.Forms", PublicKey = "0x00000000000000000400000000000000")]
	public object GetContextInfo()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public static IntPtr GetHalftonePalette()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(IntPtr);
	}

	[SecurityPermission(SecurityAction.LinkDemand, Flags = SecurityPermissionFlag.UnmanagedCode)]
	public IntPtr GetHdc()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(IntPtr);
	}

	public Color GetNearestColor(Color color)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(Color);
	}

	public void IntersectClip(Rectangle rect)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void IntersectClip(RectangleF rect)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void IntersectClip(Region region)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public bool IsVisible(Point point)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public bool IsVisible(PointF point)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public bool IsVisible(Rectangle rect)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public bool IsVisible(RectangleF rect)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public bool IsVisible(int x, int y)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public bool IsVisible(int x, int y, int width, int height)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public bool IsVisible(float x, float y)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public bool IsVisible(float x, float y, float width, float height)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public Region[] MeasureCharacterRanges(string text, Font font, RectangleF layoutRect, StringFormat stringFormat)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public SizeF MeasureString(string text, Font font)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(SizeF);
	}

	public SizeF MeasureString(string text, Font font, PointF origin, StringFormat stringFormat)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(SizeF);
	}

	public SizeF MeasureString(string text, Font font, SizeF layoutArea)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(SizeF);
	}

	public SizeF MeasureString(string text, Font font, SizeF layoutArea, StringFormat stringFormat)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(SizeF);
	}

	public SizeF MeasureString(string text, Font font, SizeF layoutArea, StringFormat stringFormat, out int charactersFitted, out int linesFilled)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(SizeF);
	}

	public SizeF MeasureString(string text, Font font, int width)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(SizeF);
	}

	public SizeF MeasureString(string text, Font font, int width, StringFormat format)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(SizeF);
	}

	public void MultiplyTransform(Matrix matrix)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void MultiplyTransform(Matrix matrix, MatrixOrder order)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	[SecurityPermission(SecurityAction.LinkDemand, Flags = SecurityPermissionFlag.UnmanagedCode)]
	public void ReleaseHdc()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	[EditorBrowsable(EditorBrowsableState.Advanced)]
	public void ReleaseHdc(IntPtr hdc)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	[EditorBrowsable(EditorBrowsableState.Never)]
	[SecurityPermission(SecurityAction.LinkDemand, Flags = SecurityPermissionFlag.UnmanagedCode)]
	public void ReleaseHdcInternal(IntPtr hdc)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void ResetClip()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void ResetTransform()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Restore(GraphicsState gstate)
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

	public GraphicsState Save()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public void ScaleTransform(float sx, float sy)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void ScaleTransform(float sx, float sy, MatrixOrder order)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void SetClip(GraphicsPath path)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void SetClip(GraphicsPath path, CombineMode combineMode)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void SetClip(Graphics g)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void SetClip(Graphics g, CombineMode combineMode)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void SetClip(Rectangle rect)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void SetClip(Rectangle rect, CombineMode combineMode)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void SetClip(RectangleF rect)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void SetClip(RectangleF rect, CombineMode combineMode)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void SetClip(Region region, CombineMode combineMode)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void TransformPoints(CoordinateSpace destSpace, CoordinateSpace srcSpace, PointF[] pts)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void TransformPoints(CoordinateSpace destSpace, CoordinateSpace srcSpace, Point[] pts)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void TranslateClip(int dx, int dy)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void TranslateClip(float dx, float dy)
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
