namespace System;

internal struct SmallRect
{
	public short Left;

	public short Top;

	public short Right;

	public short Bottom;

	public SmallRect(int left, int top, int right, int bottom)
	{
		Left = (short)left;
		Top = (short)top;
		Right = (short)right;
		Bottom = (short)bottom;
	}
}
