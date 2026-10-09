using System.Drawing.Drawing2D;
using System.Runtime.InteropServices;
using Unity;

namespace System.Drawing.Imaging;

[StructLayout(LayoutKind.Sequential)]
public sealed class ImageAttributes : ICloneable, IDisposable
{
	public ImageAttributes()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void ClearBrushRemapTable()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void ClearColorKey()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void ClearColorKey(ColorAdjustType type)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void ClearColorMatrix()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void ClearColorMatrix(ColorAdjustType type)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void ClearGamma()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void ClearGamma(ColorAdjustType type)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void ClearNoOp()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void ClearNoOp(ColorAdjustType type)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void ClearOutputChannel()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void ClearOutputChannel(ColorAdjustType type)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void ClearOutputChannelColorProfile()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void ClearOutputChannelColorProfile(ColorAdjustType type)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void ClearRemapTable()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void ClearRemapTable(ColorAdjustType type)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void ClearThreshold()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void ClearThreshold(ColorAdjustType type)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public object Clone()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public void Dispose()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void GetAdjustedPalette(ColorPalette palette, ColorAdjustType type)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void SetBrushRemapTable(ColorMap[] map)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void SetColorKey(Color colorLow, Color colorHigh)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void SetColorKey(Color colorLow, Color colorHigh, ColorAdjustType type)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void SetColorMatrices(ColorMatrix newColorMatrix, ColorMatrix grayMatrix)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void SetColorMatrices(ColorMatrix newColorMatrix, ColorMatrix grayMatrix, ColorMatrixFlag flags)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void SetColorMatrices(ColorMatrix newColorMatrix, ColorMatrix grayMatrix, ColorMatrixFlag mode, ColorAdjustType type)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void SetColorMatrix(ColorMatrix newColorMatrix)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void SetColorMatrix(ColorMatrix newColorMatrix, ColorMatrixFlag flags)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void SetColorMatrix(ColorMatrix newColorMatrix, ColorMatrixFlag mode, ColorAdjustType type)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void SetGamma(float gamma)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void SetGamma(float gamma, ColorAdjustType type)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void SetNoOp()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void SetNoOp(ColorAdjustType type)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void SetOutputChannel(ColorChannelFlag flags)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void SetOutputChannel(ColorChannelFlag flags, ColorAdjustType type)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void SetOutputChannelColorProfile(string colorProfileFilename)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void SetOutputChannelColorProfile(string colorProfileFilename, ColorAdjustType type)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void SetRemapTable(ColorMap[] map)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void SetRemapTable(ColorMap[] map, ColorAdjustType type)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void SetThreshold(float threshold)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void SetThreshold(float threshold, ColorAdjustType type)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void SetWrapMode(WrapMode mode)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void SetWrapMode(WrapMode mode, Color color)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void SetWrapMode(WrapMode mode, Color color, bool clamp)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
