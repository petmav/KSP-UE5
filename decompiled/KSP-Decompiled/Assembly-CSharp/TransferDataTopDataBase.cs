using System;
using KSP.UI;

[Serializable]
public abstract class TransferDataTopDataBase : AppUI_Data
{
	public Callback dataChangedCallback;

	public TransferDataTopDataBase(Callback dataChangedCallback)
	{
		this.dataChangedCallback = dataChangedCallback;
	}
}
