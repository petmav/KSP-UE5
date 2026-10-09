using Unity;

namespace System.Drawing.Printing;

public abstract class PrintController
{
	public virtual bool IsPreview
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	protected PrintController()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public virtual void OnEndPage(PrintDocument document, PrintPageEventArgs e)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public virtual void OnEndPrint(PrintDocument document, PrintEventArgs e)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public virtual Graphics OnStartPage(PrintDocument document, PrintPageEventArgs e)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public virtual void OnStartPrint(PrintDocument document, PrintEventArgs e)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
