using Unity;

namespace System.Drawing.Drawing2D;

public sealed class GraphicsPathIterator : MarshalByRefObject, IDisposable
{
	public int Count
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
	}

	public int SubpathCount
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(int);
		}
	}

	public GraphicsPathIterator(GraphicsPath path)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public int CopyData(ref PointF[] points, ref byte[] types, int startIndex, int endIndex)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public void Dispose()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public int Enumerate(ref PointF[] points, ref byte[] types)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public bool HasCurve()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public int NextMarker(GraphicsPath path)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public int NextMarker(out int startIndex, out int endIndex)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public int NextPathType(out byte pathType, out int startIndex, out int endIndex)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public int NextSubpath(GraphicsPath path, out bool isClosed)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public int NextSubpath(out int startIndex, out int endIndex, out bool isClosed)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(int);
	}

	public void Rewind()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
