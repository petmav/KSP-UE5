using System.ComponentModel;
using System.Globalization;
using System.Runtime.InteropServices;

namespace System.Drawing;

[Serializable]
[ComVisible(true)]
public struct Point
{
	private int x;

	private int y;

	public static readonly Point Empty;

	[Browsable(false)]
	public bool IsEmpty
	{
		get
		{
			if (x == 0)
			{
				return y == 0;
			}
			return false;
		}
	}

	public int X
	{
		get
		{
			return x;
		}
		set
		{
			x = value;
		}
	}

	public int Y
	{
		get
		{
			return y;
		}
		set
		{
			y = value;
		}
	}

	public static Point Ceiling(PointF value)
	{
		checked
		{
			int num = (int)Math.Ceiling(value.X);
			int num2 = (int)Math.Ceiling(value.Y);
			return new Point(num, num2);
		}
	}

	public static Point Round(PointF value)
	{
		checked
		{
			int num = (int)Math.Round(value.X);
			int num2 = (int)Math.Round(value.Y);
			return new Point(num, num2);
		}
	}

	public static Point Truncate(PointF value)
	{
		checked
		{
			int num = (int)value.X;
			int num2 = (int)value.Y;
			return new Point(num, num2);
		}
	}

	public static Point operator +(Point pt, Size sz)
	{
		return new Point(pt.X + sz.Width, pt.Y + sz.Height);
	}

	public static bool operator ==(Point left, Point right)
	{
		if (left.X == right.X)
		{
			return left.Y == right.Y;
		}
		return false;
	}

	public static bool operator !=(Point left, Point right)
	{
		if (left.X == right.X)
		{
			return left.Y != right.Y;
		}
		return true;
	}

	public static Point operator -(Point pt, Size sz)
	{
		return new Point(pt.X - sz.Width, pt.Y - sz.Height);
	}

	public static explicit operator Size(Point p)
	{
		return new Size(p.X, p.Y);
	}

	public static implicit operator PointF(Point p)
	{
		return new PointF(p.X, p.Y);
	}

	public Point(int dw)
	{
		y = dw >> 16;
		x = (short)(dw & 0xFFFF);
	}

	public Point(Size sz)
	{
		x = sz.Width;
		y = sz.Height;
	}

	public Point(int x, int y)
	{
		this.x = x;
		this.y = y;
	}

	public override bool Equals(object obj)
	{
		if (!(obj is Point))
		{
			return false;
		}
		return this == (Point)obj;
	}

	public override int GetHashCode()
	{
		return x ^ y;
	}

	public void Offset(int dx, int dy)
	{
		x += dx;
		y += dy;
	}

	public override string ToString()
	{
		return $"{{X={x.ToString(CultureInfo.InvariantCulture)},Y={y.ToString(CultureInfo.InvariantCulture)}}}";
	}

	public static Point Add(Point pt, Size sz)
	{
		return new Point(pt.X + sz.Width, pt.Y + sz.Height);
	}

	public void Offset(Point p)
	{
		Offset(p.X, p.Y);
	}

	public static Point Subtract(Point pt, Size sz)
	{
		return new Point(pt.X - sz.Width, pt.Y - sz.Height);
	}
}
