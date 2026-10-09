using System.ComponentModel;
using System.Security.Permissions;
using Unity;

namespace System.Drawing.Design;

[PermissionSet(SecurityAction.InheritanceDemand, Name = "FullTrust")]
[PermissionSet(SecurityAction.LinkDemand, Name = "FullTrust")]
public class UITypeEditor
{
	public virtual bool IsDropDownResizable
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	public UITypeEditor()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public virtual object EditValue(ITypeDescriptorContext context, IServiceProvider provider, object value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public object EditValue(IServiceProvider provider, object value)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public UITypeEditorEditStyle GetEditStyle()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(UITypeEditorEditStyle);
	}

	public virtual UITypeEditorEditStyle GetEditStyle(ITypeDescriptorContext context)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(UITypeEditorEditStyle);
	}

	public bool GetPaintValueSupported()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public virtual bool GetPaintValueSupported(ITypeDescriptorContext context)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return default(bool);
	}

	public virtual void PaintValue(PaintValueEventArgs e)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void PaintValue(object value, Graphics canvas, Rectangle rectangle)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
