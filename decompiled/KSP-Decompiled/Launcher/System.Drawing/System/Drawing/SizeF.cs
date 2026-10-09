using System.ComponentModel;
using System.Globalization;
using System.Runtime.InteropServices;

namespace System.Drawing;

[Serializable]
[ComVisible(true)]
public struct SizeF
{
	private float width;

	private float height;

	public static readonly SizeF Empty;

	[Browsable(false)]
	public bool IsEmpty
	{
		get
		{
			if ((double)width == 0.0)
			{
				return (double)height == 0.0;
			}
			return false;
		}
	}

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

	public static SizeF operator +(SizeF sz1, SizeF sz2)
	{
		return new SizeF(sz1.Width + sz2.Width, sz1.Height + sz2.Height);
	}

	public static bool operator ==(SizeF sz1, SizeF sz2)
	{
		if (sz1.Width == sz2.Width)
		{
			return sz1.Height == sz2.Height;
		}
		return false;
	}

	public static bool operator !=(SizeF sz1, SizeF sz2)
	{
		if (sz1.Width == sz2.Width)
		{
			return sz1.Height != sz2.Height;
		}
		return true;
	}

	public static SizeF operator -(SizeF sz1, SizeF sz2)
	{
		return new SizeF(sz1.Width - sz2.Width, sz1.Height - sz2.Height);
	}

	public static explicit operator PointF(SizeF size)
	{
		return new PointF(size.Width, size.Height);
	}

	public SizeF(PointF pt)
	{
		width = pt.X;
		height = pt.Y;
	}

	public SizeF(SizeF size)
	{
		width = size.Width;
		height = size.Height;
	}

	public SizeF(float width, float height)
	{
		this.width = width;
		this.height = height;
	}

	public override bool Equals(object obj)
	{
		if (!(obj is SizeF))
		{
			return false;
		}
		return this == (SizeF)obj;
	}

	public override int GetHashCode()
	{
		return (int)width ^ (int)height;
	}

	public PointF ToPointF()
	{
		return new PointF(width, height);
	}

	public Size ToSize()
	{
		checked
		{
			int num = (int)width;
			int num2 = (int)height;
			return new Size(num, num2);
		}
	}

	public override string ToString()
	{
		return $"{{Width={width.ToString(CultureInfo.CurrentCulture)}, Height={height.ToString(CultureInfo.CurrentCulture)}}}";
	}

	public static SizeF Add(SizeF sz1, SizeF sz2)
	{
		return new SizeF(sz1.Width + sz2.Width, sz1.Height + sz2.Height);
	}

	public static SizeF Subtract(SizeF sz1, SizeF sz2)
	{
		return new SizeF(sz1.Width - sz2.Width, sz1.Height - sz2.Height);
	}
}
