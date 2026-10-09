using Unity;

namespace System.Drawing.Printing;

public class PreviewPrintController : PrintController
{
	public override bool IsPreview
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
	}

	public virtual bool UseAntiAlias
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return default(bool);
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public PreviewPrintController()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public PreviewPageInfo[] GetPreviewPageInfo()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public override void OnEndPage(PrintDocument document, PrintPageEventArgs e)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public override void OnEndPrint(PrintDocument document, PrintEventArgs e)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public override Graphics OnStartPage(PrintDocument document, PrintPageEventArgs e)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
		return null;
	}

	public override void OnStartPrint(PrintDocument document, PrintEventArgs e)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
