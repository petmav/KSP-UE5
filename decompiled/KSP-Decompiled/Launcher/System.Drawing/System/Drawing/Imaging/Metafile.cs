using System.ComponentModel;
using System.Drawing.Design;
using System.IO;
using Unity;

namespace System.Drawing.Imaging;

[Serializable]
[Editor("System.Drawing.Design.MetafileEditor, System.Drawing.Design, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a", typeof(UITypeEditor))]
public sealed class Metafile : Image
{
	public Metafile(IntPtr henhmetafile, bool deleteEmf)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Metafile(IntPtr referenceHdc, EmfType emfType)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Metafile(IntPtr referenceHdc, EmfType emfType, string description)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Metafile(IntPtr hmetafile, WmfPlaceableFileHeader wmfHeader)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Metafile(IntPtr hmetafile, WmfPlaceableFileHeader wmfHeader, bool deleteWmf)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Metafile(IntPtr referenceHdc, Rectangle frameRect)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Metafile(IntPtr referenceHdc, Rectangle frameRect, MetafileFrameUnit frameUnit)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Metafile(IntPtr referenceHdc, Rectangle frameRect, MetafileFrameUnit frameUnit, EmfType type)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Metafile(IntPtr referenceHdc, Rectangle frameRect, MetafileFrameUnit frameUnit, EmfType type, string desc)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Metafile(IntPtr referenceHdc, RectangleF frameRect)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Metafile(IntPtr referenceHdc, RectangleF frameRect, MetafileFrameUnit frameUnit)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Metafile(IntPtr referenceHdc, RectangleF frameRect, MetafileFrameUnit frameUnit, EmfType type)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Metafile(IntPtr referenceHdc, RectangleF frameRect, MetafileFrameUnit frameUnit, EmfType type, string description)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Metafile(Stream stream)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Metafile(Stream stream, IntPtr referenceHdc)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Metafile(Stream stream, IntPtr referenceHdc, EmfType type)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Metafile(Stream stream, IntPtr referenceHdc, EmfType type, string description)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Metafile(Stream stream, IntPtr referenceHdc, Rectangle frameRect)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Metafile(Stream stream, IntPtr referenceHdc, Rectangle frameRect, MetafileFrameUnit frameUnit)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Metafile(Stream stream, IntPtr referenceHdc, Rectangle frameRect, MetafileFrameUnit frameUnit, EmfType type)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Metafile(Stream stream, IntPtr referenceHdc, Rectangle frameRect, MetafileFrameUnit frameUnit, EmfType type, string description)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Metafile(Stream stream, IntPtr referenceHdc, RectangleF frameRect)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Metafile(Stream stream, IntPtr referenceHdc, RectangleF frameRect, MetafileFrameUnit frameUnit)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Metafile(Stream stream, IntPtr referenceHdc, RectangleF frameRect, MetafileFrameUnit frameUnit, EmfType type)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Metafile(Stream stream, IntPtr referenceHdc, RectangleF frameRect, MetafileFrameUnit frameUnit, EmfType type, string description)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Metafile(string filename)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Metafile(string fileName, IntPtr referenceHdc)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Metafile(string fileName, IntPtr referenceHdc, EmfType type)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Metafile(string fileName, IntPtr referenceHdc, EmfType type, string description)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Metafile(string fileName, IntPtr referenceHdc, Rectangle frameRect)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Metafile(string fileName, IntPtr referenceHdc, Rectangle frameRect, MetafileFrameUnit frameUnit)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Metafile(string fileName, IntPtr referenceHdc, Rectangle frameRect, MetafileFrameUnit frameUnit, EmfType type)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Metafile(string fileName, IntPtr referenceHdc, Rectangle frameRect, MetafileFrameUnit frameUnit, EmfType type, string description)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Metafile(string fileName, IntPtr referenceHdc, Rectangle frameRect, MetafileFrameUnit frameUnit, string description)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Metafile(string fileName, IntPtr referenceHdc, RectangleF frameRect)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Metafile(string fileName, IntPtr referenceHdc, RectangleF frameRect, MetafileFrameUnit frameUnit)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Metafile(string fileName, IntPtr referenceHdc, RectangleF frameRect, MetafileFrameUnit frameUnit, EmfType type)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Metafile(string fileName, IntPtr referenceHdc, RectangleF frameRect, MetafileFrameUnit frameUnit, EmfType type, string description)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public Metafile(string fileName, IntPtr referenceHdc, RectangleF frameRect, MetafileFrameUnit frameUnit, string desc)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public IntPtr GetHenhmetafile()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(IntPtr);
	}

	public MetafileHeader GetMetafileHeader()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public static MetafileHeader GetMetafileHeader(IntPtr henhmetafile)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public static MetafileHeader GetMetafileHeader(IntPtr hmetafile, WmfPlaceableFileHeader wmfHeader)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public static MetafileHeader GetMetafileHeader(Stream stream)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public static MetafileHeader GetMetafileHeader(string fileName)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public void PlayRecord(EmfPlusRecordType recordType, int flags, int dataSize, byte[] data)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
