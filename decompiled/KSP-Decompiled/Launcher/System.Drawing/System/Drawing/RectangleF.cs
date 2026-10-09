using System.ComponentModel;

namespace System.Drawing;

[Serializable]
public struct RectangleF
{
	private float x;

	private float y;

	private float width;

	private float height;

	public static readonly RectangleF Empty;

	[Browsable(false)]
	public float Bottom => Y + Height;

	public float Height
	{
		get
		{
			return height;
		}
		set
		{
			height = value;
		}
	}

	[Browsable(false)]
	public bool IsEmpty
	{
		get
		{
			if (!(width <= 0f))
			{
				return height <= 0f;
			}
			return true;
		}
	}

	[Browsable(false)]
	public float Left => X;

	[Browsable(false)]
	public PointF Location
	{
		get
		{
			return new PointF(x, y);
		}
		set
		{
			x = value.X;
			y = value.Y;
		}
	}

	[Browsable(false)]
	public float Right => X + Width;

	[Browsable(false)]
	public SizeF Size
	{
		get
		{
			return new SizeF(width, height);
		}
		set
		{
			width = value.Width;
			height = value.Height;
		}
	}

	[Browsable(false)]
	public float Top => Y;

	public float Width
	{
		get
		{
			return width;
		}
		set
		{
			width = value;
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

	public static RectangleF FromLTRB(float left, float top, float right, float bottom)
	{
		return new RectangleF(left, top, right - left, bottom - top);
	}

	public static RectangleF Inflate(RectangleF rect, float x, float y)
	{
		RectangleF result = new RectangleF(rect.X, rect.Y, rect.Width, rect.Height);
		result.Inflate(x, y);
		return result;
	}

	public void Inflate(float x, float y)
	{
		Inflate(new SizeF(x, y));
	}

	public void Inflate(SizeF size)
	{
		x -= size.Width;
		y -= size.Height;
		width += size.Width * 2f;
		height += size.Height * 2f;
	}

	public static RectangleF Intersect(RectangleF a, RectangleF b)
	{
		if (!a.IntersectsWithInclusive(b))
		{
			return Empty;
		}
		return FromLTRB(Math.Max(a.Left, b.Left), Math.Max(a.Top, b.Top), Math.Min(a.Right, b.Right), Math.Min(a.Bottom, b.Bottom));
	}

	public void Intersect(RectangleF rect)
	{
		this = Intersect(this, rect);
	}

	public static RectangleF Union(RectangleF a, RectangleF b)
	{
		return FromLTRB(Math.Min(a.Left, b.Left), Math.Min(a.Top, b.Top), Math.Max(a.Right, b.Right), Math.Max(a.Bottom, b.Bottom));
	}

	public static bool operator ==(RectangleF left, RectangleF right)
	{
		if (left.X == right.X && left.Y == right.Y && left.Width == right.Width)
		{
			return left.Height == right.Height;
		}
		return false;
	}

	public static bool operator !=(RectangleF left, RectangleF right)
	{
		if (left.X == right.X && left.Y == right.Y && left.Width == right.Width)
		{
			return left.Height != right.Height;
		}
		return true;
	}

	public static implicit operator RectangleF(Rectangle r)
	{
		return new RectangleF(r.X, r.Y, r.Width, r.Height);
	}

	public RectangleF(PointF location, SizeF size)
	{
		x = location.X;
		y = location.Y;
		width = size.Width;
		height = size.Height;
	}

	public RectangleF(float x, float y, float width, float height)
	{
		this.x = x;
		this.y = y;
		this.width = width;
		this.height = height;
	}

	public bool Contains(float x, float y)
	{
		if (x >= Left && x < Right && y >= Top)
		{
			return y < Bottom;
		}
		return false;
	}

	public bool Contains(PointF pt)
	{
		return Contains(pt.X, pt.Y);
	}

	public bool Contains(RectangleF rect)
	{
		if (X <= rect.X && Right >= rect.Right && Y <= rect.Y)
		{
			return Bottom >= rect.Bottom;
		}
		return false;
	}

	public override bool Equals(object obj)
	{
		if (!(obj is RectangleF))
		{
			return false;
		}
		return this == (RectangleF)obj;
	}

	public override int GetHashCode()
	{
		return (int)(x + y + width + height);
	}

	public bool IntersectsWith(RectangleF rect)
	{
		if (!(Left >= rect.Right) && !(Right <= rect.Left) && !(Top >= rect.Bottom))
		{
			return !(Bottom <= rect.Top);
		}
		return false;
	}

	private bool IntersectsWithInclusive(RectangleF r)
	{
		if (!(Left > r.Right) && !(Right < r.Left) && !(Top > r.Bottom))
		{
			return !(Bottom < r.Top);
		}
		return false;
	}

	public void Offset(float x, float y)
	{
		X += x;
		Y += y;
	}

	public void Offset(PointF pos)
	{
		Offset(pos.X, pos.Y);
	}

	public override string ToString()
	{
		return $"{{X={x},Y={y},Width={width},Height={height}}}";
	}
}
