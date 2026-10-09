using System.ComponentModel;
using System.Globalization;
using System.Runtime.InteropServices;

namespace System.Drawing;

[Serializable]
[ComVisible(true)]
public struct PointF
{
	private float x;

	private float y;

	public static readonly PointF Empty;

	[Browsable(false)]
	public bool IsEmpty
	{
		get
		{
			if ((double)x == 0.0)
			{
				return (double)y == 0.0;
			}
			return false;
		}
	}

	public float X
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

	public float Y
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

	public static PointF operator +(PointF pt, Size sz)
	{
		return new PointF(pt.X + (float)sz.Width, pt.Y + (float)sz.Height);
	}

	public static PointF operator +(PointF pt, SizeF sz)
	{
		return new PointF(pt.X + sz.Width, pt.Y + sz.Height);
	}

	public static bool operator ==(PointF left, PointF right)
	{
		if (left.X == right.X)
		{
			return left.Y == right.Y;
		}
		return false;
	}

	public static bool operator !=(PointF left, PointF right)
	{
		if (left.X == right.X)
		{
			return left.Y != right.Y;
		}
		return true;
	}

	public static PointF operator -(PointF pt, Size sz)
	{
		return new PointF(pt.X - (float)sz.Width, pt.Y - (float)sz.Height);
	}

	public static PointF operator -(PointF pt, SizeF sz)
	{
		return new PointF(pt.X - sz.Width, pt.Y - sz.Height);
	}

	public PointF(float x, float y)
	{
		this.x = x;
		this.y = y;
	}

	public override bool Equals(object obj)
	{
		if (!(obj is PointF))
		{
			return false;
		}
		return this == (PointF)obj;
	}

	public override int GetHashCode()
	{
		return (int)x ^ (int)y;
	}

	public override string ToString()
	{
		return $"{{X={x.ToString(CultureInfo.CurrentCulture)}, Y={y.ToString(CultureInfo.CurrentCulture)}}}";
	}

	public static PointF Add(PointF pt, Size sz)
	{
		return new PointF(pt.X + (float)sz.Width, pt.Y + (float)sz.Height);
	}

	public static PointF Add(PointF pt, SizeF sz)
	{
		return new PointF(pt.X + sz.Width, pt.Y + sz.Height);
	}

	public static PointF Subtract(PointF pt, Size sz)
	{
		return new PointF(pt.X - (float)sz.Width, pt.Y - (float)sz.Height);
	}

	public static PointF Subtract(PointF pt, SizeF sz)
	{
		return new PointF(pt.X - sz.Width, pt.Y - sz.Height);
	}
}
