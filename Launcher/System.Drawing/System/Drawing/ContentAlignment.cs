using System.ComponentModel;
using System.Drawing.Design;

namespace System.Drawing;

[Editor("System.Drawing.Design.ContentAlignmentEditor, System.Drawing.Design, Version=4.0.0.0, Culture=neutral, PublicKeyToken=b03f5f7f11d50a3a", typeof(UITypeEditor))]
public enum ContentAlignment
{
	BottomCenter = 512,
	BottomLeft = 256,
	BottomRight = 1024,
	MiddleCenter = 32,
	MiddleLeft = 16,
	MiddleRight = 64,
	TopCenter = 2,
	TopLeft = 1,
	TopRight = 4
}
