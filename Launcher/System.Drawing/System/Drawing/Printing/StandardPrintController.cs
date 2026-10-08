using Unity;

namespace System.Drawing.Printing;

public class StandardPrintController : PrintController
{
	public StandardPrintController()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
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
