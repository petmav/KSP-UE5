using KSP.UI.Screens;

public class FundsWidget : CurrencyWidget
{
	public KSP.UI.Screens.Tumbler tumblers;

	private void Awake()
	{
		GameEvents.OnFundsChanged.Add(onFundsChanged);
	}

	private void OnDestroy()
	{
		GameEvents.OnFundsChanged.Remove(onFundsChanged);
	}

	private void onFundsChanged(double funds, TransactionReasons reason)
	{
		tumblers.SetValue(funds);
	}

	public override void DelayedStart()
	{
		if (Funding.Instance != null)
		{
			onFundsChanged(Funding.Instance.Funds, TransactionReasons.None);
		}
	}

	public override bool OnAboutToStart()
	{
		if (HighLogic.CurrentGame != null)
		{
			return Funding.Instance != null;
		}
		return false;
	}
}
