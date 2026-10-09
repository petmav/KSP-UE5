using System.ComponentModel;
using System.Runtime.InteropServices;

namespace System.Drawing;

[Serializable]
[ComVisible(true)]
public struct Size
{
	private int width;

	private int height;

	public static readonly Size Empty;

	[Browsable(false)]
	public bool IsEmpty
	{
		get
		{
			if (width == 0)
			{
				return height == 0;
			}
			return false;
		}
	}

	public int Width
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

	public int Height
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

	public static Size Ceiling(SizeF value)
	{
		checked
		{
			int num = (int)Math.Ceiling(value.Width);
			int num2 = (int)Math.Ceiling(value.Height);
			return new Size(num, num2);
		}
	}

	public static Size Round(SizeF value)
	{
		checked
		{
			int num = (int)Math.Round(value.Width);
			int num2 = (int)Math.Round(value.Height);
			return new Size(num, num2);
		}
	}

	public static Size Truncate(SizeF value)
	{
		checked
		{
			int num = (int)value.Width;
			int num2 = (int)value.Height;
			return new Size(num, num2);
		}
	}

	public static Size operator +(Size sz1, Size sz2)
	{
		return new Size(sz1.Width + sz2.Width, sz1.Height + sz2.Height);
	}

	public static bool operator ==(Size sz1, Size sz2)
	{
		if (sz1.Width == sz2.Width)
		{
			return sz1.Height == sz2.Height;
		}
		return false;
	}

	public static bool operator !=(Size sz1, Size sz2)
	{
		if (sz1.Width == sz2.Width)
		{
			return sz1.Height != sz2.Height;
		}
		return true;
	}

	public static Size operator -(Size sz1, Size sz2)
	{
		return new Size(sz1.Width - sz2.Width, sz1.Height - sz2.Height);
	}

	public static explicit operator Point(Size size)
	{
		return new Point(size.Width, size.Height);
	}

	public static implicit operator SizeF(Size p)
	{
		return new SizeF(p.Width, p.Height);
	}

	public Size(Point pt)
	{
		width = pt.X;
		height = pt.Y;
	}

	public Size(int width, int height)
	{
		this.width = width;
		this.height = height;
	}

	public override bool Equals(object obj)
	{
		if (!(obj is Size))
		{
			return false;
		}
		return this == (Size)obj;
	}

	public override int GetHashCode()
	{
		return width ^ height;
	}

	public override string ToString()
	{
		return $"{{Width={width}, Height={height}}}";
	}

	public static Size Add(Size sz1, Size sz2)
	{
		return new Size(sz1.Width + sz2.Width, sz1.Height + sz2.Height);
	}

	public static Size Subtract(Size sz1, Size sz2)
	{
		return new Size(sz1.Width - sz2.Width, sz1.Height - sz2.Height);
	}
}
