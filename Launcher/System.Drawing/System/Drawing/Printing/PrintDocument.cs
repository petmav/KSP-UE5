using System.ComponentModel;
using Unity;

namespace System.Drawing.Printing;

[DefaultProperty("DocumentName")]
[DefaultEvent("PrintPage")]
[ToolboxItemFilter("System.Drawing.Printing")]
public class PrintDocument : Component
{
	public PageSettings DefaultPageSettings
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public string DocumentName
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public bool OriginAtMargins
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

	public PrintController PrintController
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public PrinterSettings PrinterSettings
	{
		get
		{
			Unity.ThrowStub.ThrowNotSupportedException();
			return null;
		}
		set
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public event PrintEventHandler BeginPrint
	{
		add
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
		remove
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public event PrintEventHandler EndPrint
	{
		add
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
		remove
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public event PrintPageEventHandler PrintPage
	{
		add
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
		remove
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public event QueryPageSettingsEventHandler QueryPageSettings
	{
		add
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
		remove
		{
			Unity.ThrowStub.ThrowNotSupportedException();
		}
	}

	public PrintDocument()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected virtual void OnBeginPrint(PrintEventArgs e)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected virtual void OnEndPrint(PrintEventArgs e)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected virtual void OnPrintPage(PrintPageEventArgs e)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	protected virtual void OnQueryPageSettings(QueryPageSettingsEventArgs e)
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}

	public void Print()
	{
		Unity.ThrowStub.ThrowNotSupportedException();
	}
}
